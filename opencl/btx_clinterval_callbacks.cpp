#include <CL/cl.h>
#include <metababel/metababel.h>

#include <assert.h>
#include <iostream>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>

#include "btx_clinterval_callbacks.hpp"

struct data_s {
  EntryState entry_state;
  std::map<hpt_t, bool> last_enqueue_entry_is_traffic;
  std::map<hp_device_t, thapi_device_id> device_to_root_device;
  std::map<hp_command_queue_t, dsd_t> command_queue_to_device;
  std::map<hp_kernel_t, std::string> kernel_to_name;
  std::map<hpt_t, fn_ts_cl_t> profiled_function_name_and_ts;
  std::map<hpt_function_name_cl_t, dsd_t> function_name_to_dsd;
  std::map<hp_event_t, tfn_ts_cl_t> event_to_function_name_and_ts;
  std::map<hp_device_t, int64_t> device_ts_to_llng_ts;
  std::map<hp_event_t, sd_t> event_result_to_start_and_delta;
};

typedef struct data_s data_t;

static inline bool clRvalIsError(cl_int &cl_rval) {
  return (cl_rval != CL_SUCCESS) && (cl_rval != CL_DEVICE_NOT_AVAILABLE);
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
                              BACKEND_OPENCL, event_class_name_striped.c_str(),
                              (ts - entry_ts), err);
}

void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname,
                             int64_t vpid, uint64_t vtid) {
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid},
                                                      ts);
}

static void exits_error_absent_callback(void *btx_handle, void *usr_data,
                                        int64_t ts,
                                        const char *event_class_name,
                                        const char *hostname, int64_t vpid,
                                        uint64_t vtid) {
  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, false);
}

static void exits_error_present_callback(void *btx_handle, void *usr_data,
                                         int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid, cl_int cl_rval) {
  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid,
                    vtid, clRvalIsError(cl_rval));
}

static void traffic_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname, int64_t vpid,
                                   uint64_t vtid, size_t size) {
  // save traffic size and entry ts for use in exit callback
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key{hostname, vpid, vtid};
  state->entry_state.set_ts(key, ts);
  state->entry_state.set_data(key, size);
  // Note: flag to deal with traffic exit calls being hard to match on
  state->last_enqueue_entry_is_traffic[key] = true;
}

// Note: handles traffic exits, but can't match easily so uses flag to
// distinguish traffic vs other clEnqueue exits
static void enqueue_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                  const char *event_class_name,
                                  const char *hostname, int64_t vpid,
                                  uint64_t vtid, cl_int errcode_ret_val) {
  auto state = static_cast<data_t *>(usr_data);
  hpt_t key{hostname, vpid, vtid};
  if (!state->last_enqueue_entry_is_traffic[key]) {
    return;
  }
  // clear flag
  state->last_enqueue_entry_is_traffic[key] = false;

  // Note: don't emit a traffic event if there was an error
  if (clRvalIsError(errcode_ret_val)) {
    return;
  }

  std::string event_class_name_stripped =
      strip_event_class_name_exit(event_class_name);
  const int64_t entry_ts = state->entry_state.get_ts(key);
  auto size = state->entry_state.get_data<size_t>(key);
  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, entry_ts,
                                 BACKEND_OPENCL,
                                 event_class_name_stripped.c_str(), size);
}

static void get_device_ids_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid, cl_int errcode_ret_val,
                                         cl_uint num_devices_val,
                                         cl_device_id *devices_vals) {
  auto state = static_cast<data_t *>(usr_data);
  if ((devices_vals != nullptr) && (errcode_ret_val == CL_SUCCESS)) {
    for (unsigned int i=0; i < num_devices_val; i++) {
      const thapi_device_id d = reinterpret_cast<thapi_device_id>(devices_vals[i]);
      state->device_to_root_device[{hostname, vpid, d}] = d;
    }
  }
}

static void create_sub_devices_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                              const char *event_class_name,
                                              const char *hostname, int64_t vpid,
                                              uint64_t vtid, cl_device_id in_device) {
  auto state = static_cast<data_t *>(usr_data);
  state->entry_state.set_data({hostname, vpid, vtid},
                              reinterpret_cast<thapi_device_id>(in_device));
}


static void create_sub_devices_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *event_class_name,
                                             const char *hostname, int64_t vpid,
                                             uint64_t vtid, cl_int errcode_ret_val,
                                             cl_uint num_devices_ret_val,
                                             cl_device_id *out_devices_vals) {
  auto state = static_cast<data_t *>(usr_data);
  if ((out_devices_vals != nullptr) && (errcode_ret_val == CL_SUCCESS)) {
    auto in_device = state->entry_state.get_data<thapi_device_id>({hostname, vpid, vtid});
    const thapi_device_id root_device =
      state->device_to_root_device[{hostname, vpid, in_device}];
    for (unsigned int i=0; i < num_devices_ret_val; i++ ) {
      const thapi_device_id d = reinterpret_cast<thapi_device_id>(out_devices_vals[i]);
      state->device_to_root_device[{hostname, vpid, d}] = root_device;
    }
  }
}

static void create_command_queue_entry_callback(void *btx_handle, void *usr_data,
                                                int64_t ts, const char *event_class_name,
                                                const char *hostname, int64_t vpid,
                                                uint64_t vtid, cl_device_id device) {
  auto state = static_cast<data_t *>(usr_data);
  state->entry_state.set_data({hostname, vpid, vtid},
                              reinterpret_cast<thapi_device_id>(device));
}

static void create_command_queue_exit_callback(void *btx_handle, void *usr_data,
                                               int64_t ts, const char *event_class_name,
                                               const char *hostname, int64_t vpid,
                                               uint64_t vtid,
                                               cl_command_queue command_queue,
                                               cl_int errcode_ret_val) {
  auto state = static_cast<data_t *>(usr_data);
  auto entry_device = state->entry_state.get_data<thapi_device_id>({hostname, vpid, vtid});
  const auto root_device = state->device_to_root_device[
                                              hp_device_t(hostname, vpid, entry_device)];
  hp_command_queue_t cq_key{hostname, vpid, command_queue};
  if (root_device != 0) {
    state->command_queue_to_device[cq_key] = dsd_t(root_device, entry_device) ;
  } else {
    state->command_queue_to_device[cq_key] = dsd_t(entry_device, 0) ;
  }
}

static void kernel_info_callback(void *btx_handle, void *usr_data, int64_t ts,
                                 const char *event_class_name, const char *hostname,
                                 int64_t vpid, uint64_t vtid, cl_kernel kernel,
                                 char *function_name) {
  auto state = static_cast<data_t *>(usr_data);
  state->kernel_to_name[{hostname, vpid, kernel}] = function_name;
}

static void launch_kernel_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid, cl_command_queue command_queue,
                                         cl_kernel kernel) {
  auto state = static_cast<data_t *>(usr_data);
  auto name = state->kernel_to_name[{hostname, vpid, kernel}];
  state->profiled_function_name_and_ts[{hostname, vpid, vtid}] = {name, ts};
  state->function_name_to_dsd[{hostname, vpid, vtid, name}] =
    state->command_queue_to_device[{hostname, vpid, command_queue}];
}

static void launch_function_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *event_class_name,
                                           const char *hostname, int64_t vpid,
                                           uint64_t vtid,
                                           cl_command_queue command_queue) {
  auto state = static_cast<data_t *>(usr_data);
  auto name_str = strip_event_class_name_entry(event_class_name);
  const char *name = name_str.c_str();
  state->profiled_function_name_and_ts[{hostname, vpid, vtid}] = {name, ts};
  state->function_name_to_dsd[{hostname, vpid, vtid, name}] =
    state->command_queue_to_device[{hostname, vpid, command_queue}];
}

static void profiling_callback(void *btx_handle, void *usr_data, int64_t ts,
                               const char *hostname, int64_t vpid,
                               uint64_t vtid, int status, cl_event event) {
  auto state = static_cast<data_t *>(usr_data);

  const hp_event_t hp_event{hostname, vpid, event};
  const auto [function_name, launch_ts] =
    state->profiled_function_name_and_ts[hpt_t(hostname, vpid, vtid)];

  if (!state->event_to_function_name_and_ts.count(hp_event)){
      state->event_to_function_name_and_ts[hp_event] =
        tfn_ts_t(vtid, function_name, launch_ts);
  } else {
    const auto [start_ts, delta] = state->event_result_to_start_and_delta[hp_event];
    const auto [device, subdevice] =
      state->function_name_to_dsd[{hostname, vpid, vtid, function_name}];

    const char *metadata = "";
    // Note: delta == 0 is used to flag error state when results event happens before
    // intial profiling event
    btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid, start_ts,
                                  BACKEND_OPENCL, function_name.c_str(), delta,
                                  device, subdevice, delta == 0, metadata);

    state->event_to_function_name_and_ts.erase(hp_event);
  }
}

static void
profiling_callback_results(void *btx_handle, void *usr_data, int64_t ts,
                           const char *hostname, int64_t vpid, uint64_t vtid,
                           cl_event event, cl_int event_command_exec_status,
                           cl_int queued_status, cl_ulong queued,
                           cl_int submit_status, cl_ulong submit,
                           cl_int start_status, cl_ulong start,
                           cl_int end_status, cl_ulong end) {
  auto state = static_cast<data_t *>(usr_data);

  bool err = clRvalIsError(event_command_exec_status) || clRvalIsError(queued_status) ||
             clRvalIsError(submit_status) || clRvalIsError(start_status) ||
             clRvalIsError(end_status);

  uint64_t delta = err ? 0 : end - start;

  const hp_event_t hp_event{hostname, vpid, event};
  if (state->event_to_function_name_and_ts.count(hp_event)) {
    const auto [thread_id, function_name, launch_ts] =
      state->event_to_function_name_and_ts[hp_event];
    const auto [device, subdevice] =
      state->function_name_to_dsd[{hostname, vpid, vtid, function_name}];

    const hp_device_t hp_device{hostname, vpid, device};
    // This need to be commented due to intel bugs where clGetEventProfilingInfo return
    // host time
    //const uint64_t start_event = state->device_ts_to_llng_ts.count(hp_device) ? state->device_ts_to_llng_ts[hp_device] + start:  ts + (start - queued);
    const uint64_t start_event_ts = launch_ts + (start - queued);

    const char *metadata = "";
    btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid, start_event_ts,
                                  BACKEND_OPENCL, function_name.c_str(), delta,
                                  device, subdevice, err, metadata);

    state->event_to_function_name_and_ts.erase(hp_event);
  } else {
    const auto [function_name, launch_ts] =
      state->profiled_function_name_and_ts[{hostname, vpid, vtid}];
    // This need to be commented due to intel bugs where clGetEventProfilingInfo return host time
    //const auto [device,subdevice] = state->function_name_to_dsd[hpt_function_name_t(hostname,process_id,thread_id, function_name)];
    //const hp_device_t hp_device{hostname, process_id, device};
    //const uint64_t start_event = state->device_ts_to_llng_ts.count(hp_device) ? state->device_ts_to_llng_ts[hp_device] + start:  ts + (start - queued);
    const uint64_t start_event_ts = launch_ts + (start - queued);

    state->event_result_to_start_and_delta[hp_event] = sd_t(start_event_ts, delta);
    state->event_to_function_name_and_ts[hp_event] =
      tfn_ts_t(vtid, function_name, launch_ts);
  }

}

// NOTE: not used because of bug in Intel opencl clGetEventProfilingInfo
// TODO: has this bug been fixed???
/*
static void device_timer_callback(void *btx_handle, void *usr_data, int64_t ts,
                                  const char *hostname, int64_t vpid, uint64_t vtid,
                                  cl_device_id device, cl_ulong device_timestamp,
                                  cl_ulong host_timestamp) {
  state->device_ts_to_llng_ts[hp_device_t(hostname, vpid, (thapi_device_id)device)] =
    ts - device_timestamp;
}
*/

#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                \
  btx_register_callbacks_##base_name(btx_handle, &base_name##_callback);


void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle,
                                              &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle,
                                            &btx_finalize_component);

  REGISTER_ASSOCIATED_CALLBACK(entries);
  REGISTER_ASSOCIATED_CALLBACK(exits_error_absent);
  REGISTER_ASSOCIATED_CALLBACK(exits_error_present);

  REGISTER_ASSOCIATED_CALLBACK(traffic_entry);
  REGISTER_ASSOCIATED_CALLBACK(enqueue_exit);

  REGISTER_ASSOCIATED_CALLBACK(get_device_ids_exit);

  REGISTER_ASSOCIATED_CALLBACK(create_sub_devices_exit);
  REGISTER_ASSOCIATED_CALLBACK(create_sub_devices_entry);

  REGISTER_ASSOCIATED_CALLBACK(kernel_info);
  REGISTER_ASSOCIATED_CALLBACK(create_command_queue_entry);
  REGISTER_ASSOCIATED_CALLBACK(create_command_queue_exit);

  REGISTER_ASSOCIATED_CALLBACK(launch_kernel_entry);
  REGISTER_ASSOCIATED_CALLBACK(launch_function_entry);

  //btx_register_usr_callbacks_lttng_ust_opencl_devices_device_timer(
  //    btx_handle, &device_timer_callback);

  // device profiling events
  btx_register_callbacks_lttng_ust_opencl_profiling_event_profiling(
      btx_handle, &profiling_callback);
  btx_register_callbacks_lttng_ust_opencl_profiling_event_profiling_results(
      btx_handle, &profiling_callback_results);
}

#undef REGISTER_ASSOCIATED_CALLBACK
