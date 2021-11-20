#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>
#include <ze_api.h>
#include <unordered_set>

typedef std::map<uintptr_t, uintptr_t> MemoryInterval;

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;

typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t> hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_queue_handle_t> hp_command_queue_t;
typedef std::tuple<hostname_t, process_id_t, ze_module_handle_t> hp_module_t;
typedef hp_device_t hpd_t;
typedef hp_event_t hpe_t;
typedef hp_kernel_t hpk_t;
typedef std::tuple<uint64_t, uint64_t> clock_lttng_device_t;

typedef std::tuple<thread_id_t, thapi_function_name, std::string, thapi_device_id, uint64_t, clock_lttng_device_t> t_tfnm_m_d_ts_cld_t;
typedef std::tuple<thapi_function_name, std::string, thapi_device_id, uint64_t> tfnm_m_d_ts_t;

typedef std::tuple<bool, uint64_t, uint64_t> event_profiling_result_t;

struct zeinterval_callbacks_state {
    // https://spec.oneapi.io/level-zero/latest/core/api.html#_CPPv4N16ze_device_uuid_t2idE
    std::unordered_map<hp_command_list_t, thapi_device_id>  command_list_to_device;
    std::unordered_map<hp_device_t, ze_device_properties_t> device_to_properties;
    std::unordered_map<hpt_t, thapi_function_name>          last_kernel;
    std::unordered_map<hp_kernel_t, thapi_function_name>    kernel_to_name;
    std::unordered_map<hp_kernel_t, thapi_function_name>    kernel_to_groupsize_str;
    std::unordered_map<hpt_function_name_t, uint64_t>       host_start;
    std::unordered_map<hp_device_t, thapi_device_id>        device_to_rootdevice;
    std::unordered_map<hpt_t, thapi_device_id>              start_device;
    std::unordered_map<hpt_t, thapi_function_name>          profiled_function_name;
    std::queue<const bt_message*>                           downstream_message_queue;

    /* State variable */
    std::unordered_map<hpt_t, size_t> last_alloc;
    std::unordered_map<hpt_t, size_t> last_free;
    std::unordered_map<hpt_t, ze_module_handle_t> last_module;

    /* Handle memory copy */
    std::unordered_map<hp_module_t, std::unordered_set<uint64_t> > module_to_module_globals;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_device;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_host;
    std::unordered_map<hp_t, MemoryInterval> rangeset_memory_shared;

    std::unordered_map<hpe_t, event_profiling_result_t> event_to_profiling_result;
    std::unordered_map<hpd_t, clock_lttng_device_t>     device_timestamps_pair_ref;
    std::unordered_map<hpt_t, tfnm_m_d_ts_t>            command_partial_payload;
    std::unordered_map<hpe_t, t_tfnm_m_d_ts_cld_t>      event_payload;
    std::unordered_map<hpd_t, thapi_device_id>          subdevice_parent;
};
