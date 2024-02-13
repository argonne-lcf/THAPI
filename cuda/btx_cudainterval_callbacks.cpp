#include <cassert>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "btx_cudainterval_callbacks.hpp"

#include "context_manager.hpp"
#include "entry_state.hpp"

#include <metababel/metababel.h>

struct data_s {
  // Need to store entry state per thread in order to use in exit callback if
  // the cuResult is a non-error. Keeps track of one data item (of any type) and
  // one ts for each thread.
  EntryState entry_state;

  // kernels (CUfunction objects) are "created" before use, at which time we
  // save the name so at execute time we know the name and can emit an event
  // with the name
  std::unordered_map<hp_kernel_t, std::string> hp_kernel_to_name;

  // At device operation launch time, save the device the operation was launched
  // on, keyed by the name, since this is not available in the device profiling
  // events
  std::unordered_map<hpt_function_name_t, CUdevice> hpt_function_name_to_dev;

  // State for device event profiling
  //
  // The callbacks should be called in this order for all valid CUDA programs,
  // and all threads, whether they use runtime API or driver API:
  //
  // 1. Context set - at least one of ctx_set_current_*_callback,
  // ctx_create_*_callback
  // ... unknown number of intervening events
  // 2. module_get_function_entry_callback (entry and exit) [kernel tasks only]
  // ... unknown number of intervening events
  // 3. (non_)?kernel_task_stream_(present|absent)_entry_callback
  // 4. profiling_callback
  // 5. (non_)?kernel_task_stream_(present|absent)_exit_callback
  // ... unknown number of intervening events
  // 6. profiling_results_callback
  //
  // The information needed to emit a thapi device event is spread out
  // throughout these calls, so we need to maintain state between them.
  //
  // Since task entry callback (3) and profiling_callback (4) always happen one
  // after another in a given thread, we just need to save the function name and
  // launch ts keyed by {hostname, process, thread}.
  //
  // In profiling_callback we can associate this information with the events, so
  // when the results are finally available it can be looked up.
  std::unordered_map<hpt_t, fn_ts_t> hpt_profiled_function_name_and_ts;
  std::unordered_map<hp_event_t, fn_ts_t> hp_event_to_function_name_and_ts;

  // Encapsulate complex context management state
  CUDAContextManager context_manager;
};

using data_t = struct data_s;

template <size_t SuffixLen>
static inline std::string _strip_event_class_name(const char *str) {
  const char *p = str + strlen("lttng_");
  while (*p++ != ':') {
  }
  return std::string{p, strlen(p) - SuffixLen};
}

static inline std::string strip_event_class_name_entry(const char *str) {
  return _strip_event_class_name<strlen("_entry")>(str);
}

static inline std::string strip_event_class_name_exit(const char *str) {
  return _strip_event_class_name<strlen("_exit")>(str);
}

static void send_host_message(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name,
                              const char *hostname, int64_t vpid, uint64_t vtid,
                              bool err) {
  std::string event_class_name_striped =
      strip_event_class_name_exit(event_class_name);
  const int64_t entry_ts = static_cast<data_t *>(usr_data)->entry_state.get_ts(
      {hostname, vpid, vtid});

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts,
                              BACKEND_CUDA, event_class_name_striped.c_str(),
                              (ts - entry_ts), err);
}

static void btx_initialize_component(void **usr_data) {
  *usr_data = new data_t;
}

static void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname,
                             int64_t vpid, uint64_t vtid) {
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid},
                                                      ts);
}

static void exits_cudaError_absent_callback(void *btx_handle, void *usr_data,
                                            int64_t ts,
                                            const char *event_class_name,
                                            const char *hostname, int64_t vpid,
                                            uint64_t vtid) {
  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, false);
}

static void exits_cudaError_present_callback(void *btx_handle, void *usr_data,
                                             int64_t ts,
                                             const char *event_class_name,
                                             const char *hostname, int64_t vpid,
                                             uint64_t vtid, CUresult cuResult) {
  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, cuResultIsError(cuResult));
}

// Note: v2 API takes size_t type for size arg
static void entries_traffic_v2_callback(void *btx_handle, void *usr_data,
                                        int64_t ts,
                                        const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, size_t size) {
  // save traffic size and entry ts for use in exit callback
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key{hostname, vpid, vtid};
  state->entry_state.set_ts(key, ts);
  state->entry_state.set_data<size_t>(key, size);
}

// Note: v1 API takes uint32_t type for size arg
static void entries_traffic_v1_callback(void *btx_handle, void *usr_data,
                                        int64_t ts,
                                        const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid, uint32_t size) {
  entries_traffic_v2_callback(btx_handle, usr_data, ts, event_class_name,
                              hostname, vpid, vtid, static_cast<size_t>(size));
}

static void exits_traffic_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname, int64_t vpid,
                                   uint64_t vtid, CUresult cuResult) {
  // Note: don't emit a traffic event if there was an error
  if (cuResultIsError(cuResult)) {
    return;
  }

  std::string event_class_name_stripped =
      strip_event_class_name_exit(event_class_name);
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key{hostname, vpid, vtid};
  const int64_t entry_ts = state->entry_state.get_ts(key);
  auto size = state->entry_state.get_data<size_t>(key);
  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, entry_ts,
                                 BACKEND_CUDA,
                                 event_class_name_stripped.c_str(), size);
}

static void profiling_callback(void *btx_handle, void *usr_data, int64_t ts,
                               const char *hostname, int64_t vpid,
                               uint64_t vtid, CUevent hStart, CUevent hStop) {
  auto state = static_cast<data_t *>(usr_data);
  auto fn_name_ts = thapi_at(state->hpt_profiled_function_name_and_ts,
                             hpt_t{hostname, vpid, vtid});
  state->hp_event_to_function_name_and_ts[{hostname, vpid, hStart, hStop}] =
      fn_name_ts;
}

static void
profiling_callback_results(void *btx_handle, void *usr_data, int64_t ts,
                           const char *hostname, int64_t vpid, uint64_t vtid,
                           CUevent hStart, CUevent hStop, CUresult startStatus,
                           CUresult stopStatus, CUresult status, float ms) {
  auto state = static_cast<data_t *>(usr_data);

  // Note: assume profiling_callback is always called before this results
  // callback
  const auto [fn_name, launch_ts] =
      thapi_at(state->hp_event_to_function_name_and_ts,
               hp_event_t{hostname, vpid, hStart, hStop});

  // Note: assume (non_)?kernel_task_stream_(absent|present)_*_callback's
  // (both entry and exit) have been called in the current thread before this
  auto dev = thapi_at(state->hpt_function_name_to_dev,
                      hpt_function_name_t{hostname, vpid, vtid, fn_name});
  bool err = cuResultIsError(startStatus) || cuResultIsError(stopStatus) ||
             cuResultIsError(status);

  // Note: THAPI uses nanoseconds
  uint64_t delta = err ? 0 : static_cast<uint64_t>(ms * 1e6);
  // Note: no subdevices in CUDA, covention is to pass the device for both
  // device and subdevice
  const char *metadata = "";
  btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid, launch_ts,
                                BACKEND_CUDA, fn_name.c_str(), delta, dev, dev,
                                err, metadata);
}

// ===============================
// Context management
// ===============================

#define MAKE_ENTRY_CALLBACK1(BASE_NAME, ARG_TYPE)                              \
  static void BASE_NAME##_callback(void *btx_handle, void *usr_data,           \
                                   int64_t ts, const char *event_class_name,   \
                                   const char *hostname, int64_t vpid,         \
                                   uint64_t vtid, ARG_TYPE arg) {              \
    auto state = static_cast<data_t *>(usr_data);                              \
    state->context_manager.BASE_NAME({hostname, vpid, vtid}, arg);             \
  }

#define MAKE_ENTRY_CALLBACK2(BASE_NAME, ARG1_TYPE, ARG2_TYPE)                  \
  static void BASE_NAME##_callback(                                            \
      void *btx_handle, void *usr_data, int64_t ts,                            \
      const char *event_class_name, const char *hostname, int64_t vpid,        \
      uint64_t vtid, ARG1_TYPE arg1, ARG2_TYPE arg2) {                         \
    auto state = static_cast<data_t *>(usr_data);                              \
    state->context_manager.BASE_NAME({hostname, vpid, vtid}, arg1, arg2);      \
  }

MAKE_ENTRY_CALLBACK1(primary_ctx_retain_entry, CUdevice)
MAKE_ENTRY_CALLBACK2(primary_ctx_retain_exit, CUresult, CUcontext)

MAKE_ENTRY_CALLBACK1(ctx_create_entry, CUdevice)
MAKE_ENTRY_CALLBACK2(ctx_create_exit, CUresult, CUcontext)

MAKE_ENTRY_CALLBACK1(ctx_destroy_entry, CUcontext)
MAKE_ENTRY_CALLBACK1(ctx_destroy_exit, CUresult)

MAKE_ENTRY_CALLBACK1(ctx_set_current_entry, CUcontext)
MAKE_ENTRY_CALLBACK1(ctx_set_current_exit, CUresult)

MAKE_ENTRY_CALLBACK1(ctx_push_current_entry, CUcontext)
MAKE_ENTRY_CALLBACK1(ctx_push_current_exit, CUresult)

MAKE_ENTRY_CALLBACK2(ctx_pop_current_exit, CUresult, CUcontext)

MAKE_ENTRY_CALLBACK2(stream_create_exit, CUresult, CUstream)

#undef MAKE_ENTRY_CALLBACK1
#undef MAKE_ENTRY_CALLBACK2

// ===============================
// END Context management
// ===============================

// ===============================
// Kernel name and device tracking
// ===============================
static void module_get_function_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, char *name) {
  auto state = static_cast<data_t *>(usr_data);
  state->entry_state.set_data({hostname, vpid, vtid}, std::string{name});
}

static void module_get_function_exit_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUresult cuResult,
    CUfunction cuFunction) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto state = static_cast<data_t *>(usr_data);
  auto kernel_name =
      state->entry_state.get_data<std::string>({hostname, vpid, vtid});
  state->hp_kernel_to_name[{hostname, vpid, cuFunction}] = kernel_name;
}

/**
 * Handle two types of tasks:
 *
 * 1. kernels with function objects and names
 * 2. other async tasks like cuMemcpy* and cuLaunchHost that don't have
 *    anassociated function object
 *
 * These are distinguished by whether f_optional function object is passed
 */
static void task_entry_helper(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name,
                              const char *hostname, int64_t vpid, uint64_t vtid,
                              CUstream cuStream,
                              const std::optional<CUfunction> f_optional = {}) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t hpt{hostname, vpid, vtid};
  auto dev = state->context_manager.get_stream_device(hpt, cuStream);

  if (f_optional.has_value()) {
    // for device launch, get the name saved when the kernel was loaded in the
    // module get function callbacks
    hp_kernel_t hp_kernel_key{hostname, vpid, f_optional.value()};
    std::string name = thapi_at(state->hp_kernel_to_name, hp_kernel_key);
    state->hpt_function_name_to_dev[{hostname, vpid, vtid, name}] = dev;
    state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
  } else {
    // for host launch and non launch tasks like cuMemcpy, the device op name
    // is just the function name
    std::string name = strip_event_class_name_entry(event_class_name);
    state->hpt_function_name_to_dev[{hostname, vpid, vtid, name}] = dev;
    state->hpt_profiled_function_name_and_ts[hpt] = fn_ts_t(name, ts);
  }
}

static void non_kernel_task_stream_present_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUstream cuStream) {
  task_entry_helper(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, cuStream);
}

static void non_kernel_task_stream_absent_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid) {
  task_entry_helper(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, nullptr);
}

static void kernel_task_stream_present_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUstream cuStream,
    CUfunction f) {
  task_entry_helper(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, cuStream, f);
}

static void kernel_task_stream_absent_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, CUfunction f) {
  task_entry_helper(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, nullptr, f);
}

// ===============================
// END Kernel name and device tracking
// ===============================

#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                \
  btx_register_callbacks_##base_name(btx_handle, &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle,
                                              &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle,
                                            &btx_finalize_component);

  // generic callbacks, for host events
  REGISTER_ASSOCIATED_CALLBACK(entries);
  REGISTER_ASSOCIATED_CALLBACK(exits_cudaError_absent);
  REGISTER_ASSOCIATED_CALLBACK(exits_cudaError_present);

  // Traffic
  REGISTER_ASSOCIATED_CALLBACK(entries_traffic_v1);
  REGISTER_ASSOCIATED_CALLBACK(entries_traffic_v2);
  REGISTER_ASSOCIATED_CALLBACK(exits_traffic);

  // device profiling events
  btx_register_callbacks_lttng_ust_cuda_profiling_event_profiling(
      btx_handle, &profiling_callback);
  btx_register_callbacks_lttng_ust_cuda_profiling_event_profiling_results(
      btx_handle, &profiling_callback_results);

  // Context handling
  REGISTER_ASSOCIATED_CALLBACK(primary_ctx_retain_entry);
  REGISTER_ASSOCIATED_CALLBACK(primary_ctx_retain_exit);

  REGISTER_ASSOCIATED_CALLBACK(ctx_create_entry);
  REGISTER_ASSOCIATED_CALLBACK(ctx_create_exit);

  REGISTER_ASSOCIATED_CALLBACK(ctx_destroy_entry);
  REGISTER_ASSOCIATED_CALLBACK(ctx_destroy_exit);

  REGISTER_ASSOCIATED_CALLBACK(ctx_set_current_entry);
  REGISTER_ASSOCIATED_CALLBACK(ctx_set_current_exit);

  REGISTER_ASSOCIATED_CALLBACK(ctx_push_current_entry);
  REGISTER_ASSOCIATED_CALLBACK(ctx_push_current_exit);

  REGISTER_ASSOCIATED_CALLBACK(ctx_pop_current_exit);

  REGISTER_ASSOCIATED_CALLBACK(stream_create_exit);

  // kernel name and device tracking
  REGISTER_ASSOCIATED_CALLBACK(module_get_function_entry);
  REGISTER_ASSOCIATED_CALLBACK(module_get_function_exit);

  REGISTER_ASSOCIATED_CALLBACK(non_kernel_task_stream_present_entry);

  REGISTER_ASSOCIATED_CALLBACK(kernel_task_stream_present_entry);

  REGISTER_ASSOCIATED_CALLBACK(non_kernel_task_stream_absent_entry);

  REGISTER_ASSOCIATED_CALLBACK(kernel_task_stream_absent_entry);
}

#undef REGISTER_ASSOCIATED_CALLBACK
