#ifndef CLINTERVAL_CALLBACKS_STATE_H
#define CLINTERVAL_CALLBACKS_STATE_H

#include "xprof_utils.h"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>


typedef std::tuple<hostname_t, process_id_t, cl_command_queue> hp_command_queue_t;
typedef std::tuple<hostname_t, process_id_t, cl_event> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, cl_kernel> hp_kernel_t;

struct clinterval_callbacks_state {
    std::unordered_map<hp_command_queue_t, dsd_t> command_queue_to_device;
    std::unordered_map<hp_event_t,tfn_ts_t> event_to_function_name_and_ts;

    std::unordered_map<hp_event_t, sd_t> event_result_to_start_and_delta;
    std::unordered_map<hp_device_t, int64_t> device_ts_to_llng_ts;
    std::unordered_map<hp_kernel_t, thapi_function_name> kernel_to_name;
    std::unordered_map<hpt_function_name_t, uint64_t> host_start;

    std::unordered_map<hp_device_t, std::string> device_to_name;
    std::unordered_map<hp_device_t, thapi_device_id> device_to_rootdevice;
    std::unordered_map<hpt_t, thapi_device_id> start_device;
    std::unordered_map<hpt_t, thapi_function_name> profiled_function_name;
    std::unordered_map<hpt_t, fn_ts_t> profiled_function_name_and_ts;
    std::unordered_map<hpt_function_name_t, dsd_t> function_name_to_dsd;

    std::queue<const bt_message*> downstream_message_queue;
};

#endif
