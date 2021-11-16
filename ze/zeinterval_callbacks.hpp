#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>
#include <ze_api.h>

typedef std::map<uintptr_t, uintptr_t> MemoryInterval;

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;

typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t> hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_queue_handle_t> hp_command_queue_t;

typedef std::tuple<uint64_t, uint64_t> timerResolution_kernelTimestampValidBits_t;
typedef std::tuple<uint64_t, uint64_t> clock_lttng_device_t;


typedef std::tuple<thapi_function_name, std::string, thapi_device_id, thapi_device_id, long, clock_lttng_device_t> fnm_dsd_ts_ld_t;
typedef std::tuple<thread_id_t, thapi_function_name, std::string, thapi_device_id, thapi_device_id, long, clock_lttng_device_t> tfnm_dsd_ts_ld_t;

struct zeinterval_callbacks_state {
    std::unordered_map<hp_event_t,tfnm_dsd_ts_ld_t> event_to_function_name_dsd_and_ts;
    std::unordered_map<hpt_t, fnm_dsd_ts_t> profiled_function_name_dsd_and_ts;
    // https://spec.oneapi.io/level-zero/latest/core/api.html#_CPPv4N16ze_device_uuid_t2idE
    std::unordered_map<h_device_t, ze_device_uuid_t> device_to_uuid;
    std::unordered_map<hp_command_list_t, dsd_t> command_list_to_device;
    std::unordered_map<hp_device_t, timerResolution_kernelTimestampValidBits_t> device_to_timerResolution_kernelTimestampValidBits;
    std::unordered_map<hp_device_t, clock_lttng_device_t> sync_clock_lttng_device;
    std::unordered_map<hpt_t, thapi_function_name> last_kernel;
    std::unordered_map<hp_kernel_t, thapi_function_name> kernel_to_name;
    std::unordered_map<hp_kernel_t, thapi_function_name> kernel_to_groupsize;
    std::unordered_map<hpt_function_name_t, uint64_t> host_start;
    std::unordered_map<hp_device_t, thapi_device_id> device_to_rootdevice;
    std::unordered_map<hpt_t, thapi_device_id> start_device;
    std::unordered_map<hpt_t, thapi_function_name> profiled_function_name;
    std::queue<const bt_message*> downstream_message_queue;
    std::unordered_map<hpt_t, size_t> last_alloc;
    std::unordered_map<hpt_t, size_t> last_free;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_device;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_host;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_shared;
};

//Memory interval
template<class T>
bool contains(const MemoryInterval& m, const T val) {
    const auto it = m.lower_bound(val);
    if (it == m.cend())
        return false;
    return (val < it->second);
}
