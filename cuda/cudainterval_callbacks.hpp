#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>
#include <stack>

#define __CUDA_API_VERSION_INTERNAL 1
#include <cuda.h>

typedef std::tuple<hostname_t, process_id_t, CUevent, CUevent> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, CUfunction> hp_kernel_t;
typedef std::tuple<hostname_t, process_id_t, CUstream> hp_stream_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, CUcontext> hpt_context_t;
typedef std::tuple<hostname_t, process_id_t, CUcontext> hp_context_t;
typedef std::stack<CUcontext>  context_stack_t;

struct cudainterval_callbacks_state {

    std::unordered_map<hp_event_t,tfn_ts_t> event_to_function_name_and_ts;
    std::unordered_map<hpt_t, context_stack_t> context_stack;   
        
    std::unordered_map<hpt_t, thapi_device_id> last_device;

    std::unordered_map<hp_stream_t, CUcontext> stream_to_context;
    std::unordered_map<hp_context_t, thapi_device_id> context_to_device;
    std::unordered_map<hp_stream_t, thapi_device_id> stream_to_device;

    std::unordered_map<hpt_t, thapi_function_name> last_kernel;
    std::unordered_map<hp_event_t, sd_t> event_result_to_start_and_delta;
    std::unordered_map<hp_kernel_t, thapi_function_name> kernel_to_name;

    std::unordered_map<hpt_function_name_t, uint64_t> host_start;

    std::unordered_map<hp_device_t, std::string> device_to_name;
    std::unordered_map<hpt_t, thapi_function_name> profiled_function_name;
    std::unordered_map<hpt_t, fn_ts_t> profiled_function_name_and_ts;
    std::unordered_map<hpt_function_name_t, thapi_device_id> function_name_to_d;

    std::queue<const bt_message*> downstream_message_queue;

};
