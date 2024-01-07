#include <cassert>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <tuple>
#include <string>
#include <unordered_map>

#include "btx_cudainterval_callbacks.hpp"

#include "context_manager.hpp"

#include <metababel/metababel.h>


struct data_s {
  // store entry state per thread in order to use in exit callback, conditioned
  // on the cuResult being a non-error
  std::unordered_map<hpt_t, int64_t> entry_ts;
  std::unordered_map<hpt_t, std::string> entry_kernel_name;
  std::unordered_map<hpt_t, size_t> entry_traffic_size;

  // kernels (CUfunction objects) are "created" before use, at which time we save
  // the name so at execute time we know the name and can emit an event with the name
  std::unordered_map<hp_kernel_t, std::string> hp_kernel_to_name;

  // At device operation launch time, save the device the operation was launched on,
  // keyed by the name, since this is not available in the device profiling events
  std::unordered_map<hpt_function_name_t, CUdevice> hpt_function_name_to_dev;

  // State for device event profiling
  //
  // The flow of events for event profiling is:
  //
  // 1. device operation entry (cuMemcpy, cuLaunch, etc)
  // 2. event_profiling
  // 3. device operation exit
  // 4. unknown number of intervening events
  // ...
  // 5. event profiling results
  // 
  // The information needed to emit a thapi device event is spread out throughout
  // these calls, so we need to maintain state between them.
  // 
  // Since 1 and 2 always happen one after another in a given thread, we just need
  // to save the function name and launch ts keyed by {hostname, process, thread}.
  // In (2) we can associate this information with the events, so when the results
  // are finally available it can be looked up.
  std::unordered_map<hpt_t, fn_ts_t> hpt_profiled_function_name_and_ts;
  std::unordered_map<hp_event_t, fn_ts_t> hp_event_to_function_name_and_ts;

  // Encapsulate complex context management state
  CUDAContextManager context_manager;
};

using data_t = struct data_s;

void set_entry_ts(data_t *data, hpt_t hpt, int64_t ts) {
  data->entry_ts[hpt] = ts;
}

int64_t get_entry_ts(data_t *data, hpt_t hpt) {
  return data->entry_ts.at(hpt);
}

std::string strip_event_class_name(const char *str) {
  std::string temp(str);
  std::smatch match;
  std::regex_search(temp, match, std::regex(":(.*?)_?(?:entry|exit)?$"));

  // The entire match is hold in the first item, sub_expressions 
  // (parentheses delimited groups) are stored after.
  assert( match.size() > 1 && "Event_class_name not matching regex.");

  return match[1].str();
}

static void send_host_message(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name, const char *hostname, int64_t vpid,
                              uint64_t vtid, bool err) {
  std::string event_class_name_striped = strip_event_class_name(event_class_name);
  const int64_t _start = get_entry_ts(static_cast<data_t *>(usr_data), {hostname, vpid, vtid});

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, _start, BACKEND_CUDA,
                              event_class_name_striped.c_str(), (ts - _start), err);
}

static void send_traffic_message(void *btx_handle, void *usr_data, int64_t ts,
                                 const char *event_class_name, const char *hostname,
                                 int64_t vpid, uint64_t vtid, bool err) {
  std::string event_class_name_stripped = strip_event_class_name(event_class_name);
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key = {hostname, vpid, vtid};
  const int64_t _start = get_entry_ts(state, key);

  if (!err) {
    auto size = state->entry_traffic_size.at(key);
    btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, _start, BACKEND_CUDA,
                                   event_class_name_stripped.c_str(), size);
  }
}


void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                      const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid) {
  set_entry_ts(((data_t *)usr_data), {hostname, vpid, vtid}, ts);
}

void exits_callback_cudaError_absent(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *event_class_name, const char *hostname,
                                     int64_t vpid, uint64_t vtid) {
  hpt_t hpt = {hostname, vpid, vtid};
  try {
    send_host_message(btx_handle, usr_data, ts, event_class_name,
                      hostname, vpid, vtid, false);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "entry ts not found for thread in exit cudaError_absent callback");
  }
}

void exits_callback_cudaError_present(void *btx_handle, void *usr_data, int64_t ts,
                                      const char *event_class_name, const char *hostname,
                                      int64_t vpid, uint64_t vtid, CUresult cuResult) {
  hpt_t hpt = {hostname, vpid, vtid};
  // Not an Error (cuResult == cudaErrorNotReady)
  bool err = (cuResult != CUDA_SUCCESS) && (cuResult != CUDA_ERROR_NOT_READY);
  try {
    send_host_message(btx_handle, usr_data, ts, event_class_name,
                      hostname, vpid, vtid, err);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "entry ts not found for thread in exit cudaError_present callback");
  }
}

void entries_traffic_callback(void *btx_handle, void *usr_data, int64_t ts,
                      const char *event_class_name, const char *hostname, int64_t vpid,
                      uint64_t vtid, size_t size) {
  // save traffic size and entry ts for use in exit callback
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key = {hostname, vpid, vtid};
  set_entry_ts(state, key, ts);
  state->entry_traffic_size[key] = size;
}

void exits_traffic_callback(void *btx_handle, void *usr_data, int64_t ts,
                            const char *event_class_name, const char *hostname,
                            int64_t vpid, uint64_t vtid, CUresult cuResult) {
  hpt_t hpt = {hostname, vpid, vtid};

  // Not an Error (cuResult == cudaErrorNotReady)
  bool err = (cuResult != CUDA_SUCCESS) && (cuResult != CUDA_ERROR_NOT_READY);
  try {
    send_traffic_message(btx_handle, usr_data, ts, event_class_name,
                         hostname, vpid, vtid, err);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "entry traffic size not found for hpt in traffic message");
  }
}

void profiling_callback(void *btx_handle, void *usr_data, int64_t ts,
                        const char *hostname, int64_t vpid, uint64_t vtid,
                        CUevent hStart, CUevent hStop) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  hp_event_t hp_event = {hostname, vpid, hStart, hStop};
#if 0
  if (state->hpt_profiled_function_name_and_ts.count(hpt) == 0) {
    // Event we are not interested in profiling
    // TODO: we could be missing things, maybe we should try to handle all cases?
    // one I've encountered so far is cuMemFree, easy to add, but there may
    // be many others.
    // Hack: mark as to ignore in results callback
    std::cerr << "No name saved for event (" << hostname
              << ", " << vpid << ")" << std::endl;
    state->hp_event_to_function_name_and_ts[hp_event] = {"", 0};
    return;
  }
#endif
  try {
    auto fn_name_ts = state->hpt_profiled_function_name_and_ts.at(hpt);
    state->hp_event_to_function_name_and_ts[hp_event] = fn_name_ts;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "profiled function not found for hp event in profiling callback");
  }
}

void profiling_callback_results(void *btx_handle, void *usr_data, int64_t ts,
                                const char *hostname, int64_t vpid, uint64_t vtid,
                                CUevent hStart, CUevent hStop,
                                CUresult startStatus, CUresult stopStatus,
                                CUresult status, float ms) {
  auto state = static_cast<data_t *>(usr_data);

  hpt_t hpt = {hostname, vpid, vtid};
  hp_event_t hp_event = {hostname, vpid, hStart, hStop};

  std::string fn_name;
  long launch_ts = 0;

  try {
    // Note: this assumes profiling_callback is always called before this results
    // callback
    const auto [fn_name_, launch_ts_] = state->hp_event_to_function_name_and_ts.at(hp_event);
    fn_name = fn_name_;
    launch_ts = launch_ts_;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "function name and ts not found for hp event in profiling results callback");
  }

#if 0
  if (fn_name == "" && launch_ts == 0) {
    // event marked for ignoring
    std::cerr << "Event marked for ignore (" << hostname
              << ", " << vpid << ")" << std::endl;
    return;
  }
#endif

  try {
    hpt_function_name_t hpt_function_name = {hostname, vpid, vtid, fn_name};
    auto dev = state->hpt_function_name_to_dev.at(hpt_function_name);
    bool err = cuResultIsError(startStatus) || cuResultIsError(stopStatus)
             || cuResultIsError(status);

    uint64_t delta = static_cast<uint64_t>(ms * 1e6);
    if (err) {
      delta = 0;
    }
    btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid, launch_ts,
                                  BACKEND_CUDA, fn_name.c_str(), delta,
                                  dev, dev, err, "");
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "device not found for hpt function name in profiling results callback");
  }
}


// ===============================
// Context management
// ===============================
void primary_ctx_retain_entry_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid, CUdevice dev) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_retain_entry(hpt, dev);
}

void primary_ctx_retain_exit_callback(void *btx_handle, void *usr_data,
                                      int64_t ts, const char *event_class_name,
                                      const char *hostname, int64_t vpid,
                                      uint64_t vtid, CUresult cuResult,
                                      CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_retain_exit(hpt, cuResult, ctx);
}

void primary_ctx_release_entry_callback(void *btx_handle, void *usr_data,
                                        int64_t ts, const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, CUdevice dev) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_release_entry(hpt, dev);
}

void primary_ctx_release_exit_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid, CUresult cuResult) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_release_exit(hpt, cuResult);
}

void primary_ctx_reset_entry_callback(void *btx_handle, void *usr_data,
                                      int64_t ts, const char *event_class_name,
                                      const char *hostname, int64_t vpid,
                                      uint64_t vtid, CUdevice dev) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_reset_entry(hpt, dev);
}

void primary_ctx_reset_exit_callback(void *btx_handle, void *usr_data,
                                     int64_t ts, const char *event_class_name,
                                     const char *hostname, int64_t vpid,
                                     uint64_t vtid, CUresult cuResult) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.primary_ctx_reset_exit(hpt, cuResult);
}

void ctx_create_entry_callback(void *btx_handle, void *usr_data,
                               int64_t ts, const char *event_class_name,
                               const char *hostname, int64_t vpid,
                               uint64_t vtid, CUdevice dev) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_create_entry(hpt, dev);
}

void ctx_create_exit_callback(void *btx_handle, void *usr_data,
                              int64_t ts, const char *event_class_name,
                              const char *hostname, int64_t vpid,
                              uint64_t vtid, CUresult cuResult,
                              CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_create_exit(hpt, cuResult, ctx);
}

void ctx_destroy_entry_callback(void *btx_handle, void *usr_data,
                                int64_t ts, const char *event_class_name,
                                const char *hostname, int64_t vpid,
                                uint64_t vtid, CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_destroy_entry(hpt, ctx);
}

void ctx_destroy_exit_callback(void *btx_handle, void *usr_data,
                                   int64_t ts, const char *event_class_name,
                                   const char *hostname, int64_t vpid,
                                   uint64_t vtid, CUresult cuResult) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_destroy_exit(hpt, cuResult);
}

void ctx_set_current_entry_callback(void *btx_handle, void *usr_data,
                                        int64_t ts, const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_set_current_entry(hpt, ctx);
}

void ctx_set_current_exit_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid, CUresult cuResult) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_set_current_exit(hpt, cuResult);
}

void ctx_push_current_entry_callback(void *btx_handle, void *usr_data,
                                         int64_t ts, const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid, CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_push_current_entry(hpt, ctx);
}

void ctx_push_current_exit_callback(void *btx_handle, void *usr_data,
                                        int64_t ts, const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, CUresult cuResult) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_push_current_exit(hpt, cuResult);
}

void ctx_pop_current_entry_callback(void *btx_handle, void *usr_data,
                                        int64_t ts, const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_pop_current_entry(hpt);
}

void ctx_pop_current_exit_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid, CUresult cuResult,
                                       CUcontext ctx) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.ctx_pop_current_exit(hpt, cuResult, ctx);
}

void stream_create_exit_callback(void *btx_handle, void *usr_data,
                                 int64_t ts, const char *event_class_name,
                                 const char *hostname, int64_t vpid,
                                 uint64_t vtid, CUresult cuResult,
                                 CUstream stream) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->context_manager.stream_create_exit(hpt, cuResult, stream);
}
// ===============================
// END Context management
// ===============================

// ===============================
// Kernel name and device tracking
// ===============================
void module_get_function_entry_callback(void *btx_handle, void *usr_data,
                                        int64_t ts, const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, char* name) {
  std::string name_str(name);
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  state->entry_kernel_name[hpt] = name_str;
}

void module_get_function_exit_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid, CUresult cuResult,
                                       CUfunction cuFunction) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  try {
    auto kernel_name = state->entry_kernel_name.at(hpt);
    hp_kernel_t hp_kernel_key = {hostname, vpid, cuFunction};
    state->hp_kernel_to_name[hp_kernel_key] = kernel_name;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "entry kernel name not found in module get function exit callback");
  }
}

// TODO: don't update name->dev map on exit failure?
void device_mem_op_stream_present_entry_callback(void *btx_handle, void *usr_data,
                                                 int64_t ts, const char *event_class_name,
                                                 const char *hostname, int64_t vpid,
                                                 uint64_t vtid, CUstream cuStream) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  CUdevice dev;
  bool found = state->context_manager.get_stream_device(hpt, cuStream, &dev);
  if (!found) {
    THAPI_FATAL_HPT(hpt,
                    "no device for thread and stream in mem op stream present entry");
  }
  std::string name = strip_event_class_name(event_class_name);
  hpt_function_name_t hpt_function_name = {hostname, vpid, vtid, name};
  state->hpt_function_name_to_dev[hpt_function_name] = dev;

  state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
}

void device_mem_op_stream_absent_entry_callback(void *btx_handle, void *usr_data,
                                                int64_t ts, const char *event_class_name,
                                                const char *hostname, int64_t vpid,
                                                uint64_t vtid) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  CUdevice dev;
  bool found = state->context_manager.get_device(hpt, &dev);
  if (!found) {
    THAPI_FATAL_HPT(hpt, "no device for thread in mem op stream absent entry");
  }
  std::string name = strip_event_class_name(event_class_name);
  hpt_function_name_t hpt_function_name = {hostname, vpid, vtid, name};
  state->hpt_function_name_to_dev[hpt_function_name] = dev;

  state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
}

// TODO: don't update name->dev map on exit failure?
void device_kernel_launch_stream_present_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUfunction f,
    CUstream cuStream) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  CUdevice dev;
  bool found = state->context_manager.get_stream_device(hpt, cuStream, &dev);
  if (!found) {
    THAPI_FATAL_HPT(hpt,
      "no device for thread and stream in kernel launch stream present entry");
  }
  hp_kernel_t hp_kernel_key = {hostname, vpid, f};
  try
  {
    std::string name = state->hp_kernel_to_name.at(hp_kernel_key);
    hpt_function_name_t hpt_function_name = {hostname, vpid, vtid, name};
    state->hpt_function_name_to_dev[hpt_function_name] = dev;

    state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "device not found for hpt function name in kernel launch stream present callback");
  }
}

void device_kernel_launch_stream_absent_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUfunction f) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt = {hostname, vpid, vtid};
  CUdevice dev;
  bool found = state->context_manager.get_device(hpt, &dev);
  if (!found) {
    THAPI_FATAL_HPT(hpt,
      "no device for thread in kernel launch stream absent entry");
  }
  hp_kernel_t hp_kernel_key = {hostname, vpid, f};
  try {
    std::string name = state->hp_kernel_to_name.at(hp_kernel_key);
    hpt_function_name_t hpt_function_name = {hostname, vpid, vtid, name};
    state->hpt_function_name_to_dev[hpt_function_name] = dev;

    state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "device not found for hpt function name in kernel launch stream absent callback");
  }
}


// ===============================
// END Kernel name and device tracking
// ===============================

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  
  // generic callbacks, for host events
  btx_register_callbacks_entries(btx_handle, &entries_callback);
  btx_register_callbacks_exits_cudaError_absent(btx_handle, &exits_callback_cudaError_absent);
  btx_register_callbacks_exits_cudaError_present(btx_handle, &exits_callback_cudaError_present);

  // Traffic
  btx_register_callbacks_entries_traffic(btx_handle, &entries_traffic_callback);
  btx_register_callbacks_exits_traffic(btx_handle, &exits_traffic_callback);

  // device profiling events
  btx_register_callbacks_lttng_ust_cuda_profiling_event_profiling(btx_handle,
                                                                  &profiling_callback);
  btx_register_callbacks_lttng_ust_cuda_profiling_event_profiling_results(
    btx_handle, &profiling_callback_results);

  // Context handling
  btx_register_callbacks_primary_ctx_retain_entry(btx_handle,
    &primary_ctx_retain_entry_callback);
  btx_register_callbacks_primary_ctx_retain_exit(btx_handle,
    &primary_ctx_retain_exit_callback);

  btx_register_callbacks_primary_ctx_release_entry(btx_handle,
    &primary_ctx_release_entry_callback);
  btx_register_callbacks_primary_ctx_release_exit(btx_handle,
    &primary_ctx_release_exit_callback);

  btx_register_callbacks_primary_ctx_reset_entry(btx_handle,
    &primary_ctx_reset_entry_callback);
  btx_register_callbacks_primary_ctx_reset_exit(btx_handle,
    &primary_ctx_reset_exit_callback);

  btx_register_callbacks_ctx_create_entry(btx_handle,
    &ctx_create_entry_callback);
  btx_register_callbacks_ctx_create_exit(btx_handle,
    &ctx_create_exit_callback);

  btx_register_callbacks_ctx_destroy_entry(btx_handle,
    &ctx_destroy_entry_callback);
  btx_register_callbacks_ctx_destroy_exit(btx_handle,
    &ctx_destroy_exit_callback);

  btx_register_callbacks_ctx_set_current_entry(btx_handle,
    &ctx_set_current_entry_callback);
  btx_register_callbacks_ctx_set_current_exit(btx_handle,
    &ctx_set_current_exit_callback);

  btx_register_callbacks_ctx_push_current_entry(btx_handle,
    &ctx_push_current_entry_callback);
  btx_register_callbacks_ctx_push_current_exit(btx_handle,
    &ctx_push_current_exit_callback);

  btx_register_callbacks_ctx_pop_current_entry(btx_handle,
    &ctx_pop_current_entry_callback);
  btx_register_callbacks_ctx_pop_current_exit(btx_handle,
    &ctx_pop_current_exit_callback);

  btx_register_callbacks_stream_create_exit(btx_handle,
    &stream_create_exit_callback);
  // END context handling

  // kernel name and device tracking
  btx_register_callbacks_module_get_function_entry(btx_handle,
    &module_get_function_entry_callback);
  btx_register_callbacks_module_get_function_exit(btx_handle,
    &module_get_function_exit_callback);

  btx_register_callbacks_device_mem_op_stream_present_entry(btx_handle,
    &device_mem_op_stream_present_entry_callback);
  btx_register_callbacks_device_mem_op_stream_absent_entry(btx_handle,
    &device_mem_op_stream_absent_entry_callback);

  btx_register_callbacks_device_kernel_launch_stream_present_entry(btx_handle,
    &device_kernel_launch_stream_present_entry_callback);
  btx_register_callbacks_device_kernel_launch_stream_absent_entry(btx_handle,
    &device_kernel_launch_stream_absent_entry_callback);
}
