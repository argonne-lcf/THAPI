#pragma once

#include "xprof_utils.hpp"
#include <cstddef> // Bytes
#include <metababel/metababel.h>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t> hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_queue_handle_t> hp_command_queue_t;

typedef std::tuple<hostname_t, process_id_t, ze_module_handle_t> hp_module_t;

typedef std::map<uintptr_t, uintptr_t> memory_interval_t;
typedef std::tuple<uint64_t, uint64_t> clock_lttng_device_t;

typedef std::tuple<zes_fabric_port_throughput_t, uint64_t> fabricPort_timestamp_t;
typedef std::tuple<zes_power_energy_counter_t, uint64_t> energy_timestamp_t;
typedef std::tuple<zes_engine_stats_t, uint64_t> engines_timestamp_t;

typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, uint32_t> hpdd_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, uint32_t> hpdsd_t;

typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_fabric_port_handle_t> hpdf_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_pwr_handle_t> hpdpwr_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_freq_handle_t> hpdfreq_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_engine_handle_t> hpdeng_t;

typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_fabric_port_handle_t, uint32_t> hpdfsd_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_engine_handle_t, uint32_t> hpdengsd_t;
typedef std::tuple<hostname_t, process_id_t, ze_device_handle_t, zes_pwr_handle_t, uint32_t> hpdpwrd_t;

using btx_kernel_group_size_t = std::tuple<uint32_t, uint32_t, uint32_t>;
using btx_kernel_desct_t =
    std::tuple<std::string /*ze_kernel_desc_t*/, ze_kernel_properties_t, btx_kernel_group_size_t>;

enum class btx_event_t { TRAFFIC, KERNEL, OTHER };
using btx_additional_info_traffic_t = std::tuple<int64_t /*ts*/, size_t /*size*/>;
using btx_additional_info_kernel_t = std::string /*metadata*/;
using btx_additional_info =
    std::variant<std::monostate, btx_additional_info_traffic_t, btx_additional_info_kernel_t>;

using btx_launch_desc_t =
    std::tuple<ze_command_list_handle_t, std::string /*name*/, int64_t /*ts min */,
               btx_event_t /* type of enum */, btx_additional_info /* additional data */>;

using btx_event_desct_t =
    std::tuple<thread_id_t, ze_command_queue_desc_t, ze_command_list_handle_t,
               bool /*hCommandListIsImmediate*/, ze_device_handle_t, std::string /*name*/,
               int64_t /*ts born min*/, clock_lttng_device_t /* clock sync pair */,
               btx_event_t /* type of enum */,
               btx_additional_info /* pointer to additional data */>;

using btx_command_list_desc_t =
    std::tuple<ze_command_queue_desc_t, ze_device_handle_t, bool /*hCommandListIsImmediate*/>;

struct data_s {
  /* Host */
  EntryState entry_state;

  std::unordered_map<hp_kernel_t, btx_kernel_desct_t> kernelToDesct;

  std::unordered_map<hp_command_list_t, btx_command_list_desc_t> commandListToBtxDesc;
  std::unordered_map<hp_command_queue_t, ze_command_queue_desc_t> commandQueueToDesc;

  std::unordered_map<hpt_t, btx_launch_desc_t> threadToLastLaunchInfo;
  std::unordered_map<hp_event_t, btx_event_desct_t> eventToBtxDesct;
  // Require for non IMM
  std::unordered_map<hp_command_list_t, std::unordered_set<ze_event_handle_t>> commandListToEvents;

  /* Handle memory copy */
  std::unordered_map<hp_module_t, std::unordered_set<uint64_t>> module_global_pointer;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_device;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_host;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_shared;
  std::unordered_map<hpt_t, std::unordered_map<hp_t, memory_interval_t> *> rangeset_tmp;

  std::unordered_map<hp_device_t, ze_device_properties_t> device_property;
  std::unordered_map<hp_device_t, thapi_device_id> subdevice_parent;

  std::unordered_map<hpt_t, std::pair<ze_device_handle_t, ze_command_queue_desc_t>> imm_tmp;

  std::unordered_map<hp_device_t, clock_lttng_device_t> device_timestamps_pair_ref;
  /* Sampling */

  std::unordered_map<hpdf_t, zes_fabric_port_properties_t> fabricPort_property;
  std::unordered_map<hpdpwr_t, zes_power_properties_t> power_property;
  std::unordered_map<hpdfreq_t, zes_freq_properties_t> frequency_property;
  std::unordered_map<hpdeng_t, zes_engine_properties_t> engine_property;

  std::unordered_map<hpdpwrd_t, energy_timestamp_t> device_energy_ref;
  std::unordered_map<hpdengsd_t, engines_timestamp_t> device_engines_ref;
  std::unordered_map<hpdfsd_t, fabricPort_timestamp_t> device_fabricPort_ref;
};
typedef struct data_s data_t;
