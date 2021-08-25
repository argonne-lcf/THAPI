#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>

#include <ze_api.h>

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;

typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t> hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_queue_handle_t> hp_command_queue_t;

typedef std::tuple<uint64_t, uint64_t> timerResolution_kernelTimestampValidBits_t;
typedef std::tuple<uint64_t, uint64_t> clock_lttng_device_t;

struct zeinterval_callbacks_state {
    std::unordered_map<hp_event_t,tfn_dsd_ts_t> event_to_function_name_dsd_and_ts;
    std::unordered_map<hpt_t, fn_dsd_ts_t> profiled_function_name_dsd_and_ts;

    std::unordered_map<hp_command_list_t, dsd_t> command_list_to_device;
    std::unordered_map<hp_device_t, timerResolution_kernelTimestampValidBits_t> device_to_timerResolution_kernelTimestampValidBits;

    std::unordered_map<hp_device_t, clock_lttng_device_t> sync_clock_lttng_device;

    std::unordered_map<hpt_t, thapi_function_name> last_kernel;

    std::unordered_map<hp_event_t, sd_t> event_result_to_start_and_delta;
    std::unordered_map<hp_device_t, int64_t> device_ts_to_llng_ts;
    std::unordered_map<hp_kernel_t, thapi_function_name> kernel_to_name;
    std::unordered_map<hpt_function_name_t, uint64_t> host_start;

    std::unordered_map<hp_device_t, std::string> device_to_name;
    std::unordered_map<hp_device_t, thapi_device_id> device_to_rootdevice;
    std::unordered_map<hpt_t, thapi_device_id> start_device;
    std::unordered_map<hpt_t, thapi_function_name> profiled_function_name;

    std::queue<const bt_message*> downstream_message_queue;

};
