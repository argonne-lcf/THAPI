#include "btx_zeinterval_callbacks.hpp"
#include "xprof_utils.hpp"
#include <cassert>
#include <iostream>
#include <metababel/metababel.h>
#include <regex>
#include <stdio.h>
#include <string>
#include <unordered_map>

/*
 * Memory Interval
 */

static void add_memory(data_t *state, hp_t hp, uintptr_t ptr, size_t size, std::string source) {
  std::unordered_map<hp_t, memory_interval_t> *mi = nullptr;
  if (source == "lttng_ust_ze:zeMemAllocHost_exit")
    mi = &state->rangeset_memory_host;
  else if (source == "lttng_ust_ze:zeMemAllocDevice_exit")
    mi = &state->rangeset_memory_device;
  else if (source == "lttng_ust_ze:zeMemAllocShared_exit")
    mi = &state->rangeset_memory_shared;
  else {
    std::cout << "WARNING Tried to add unrecognized memory " << source << "ptr " << ptr
              << std::endl;
    return;
  }
  (*mi)[hp][ptr] = ptr + size;
}

static size_t remove_memory(data_t *state, hp_t hp, const uintptr_t ptr) {
  auto &rs = state->rangeset_memory_shared[hp];
  auto its = rs.find(ptr);
  if (its != rs.end()) {
    size_t tmp = its->second - its->first;
    rs.erase(its);
    return tmp;
  }
  auto &rd = state->rangeset_memory_device[hp];
  auto itd = rd.find(ptr);
  if (itd != rd.end()) {
    size_t tmp = itd->second - itd->first;
    rd.erase(itd);
    return tmp;
  }
  auto &rh = state->rangeset_memory_host[hp];
  auto ith = rh.find(ptr);
  if (ith != rh.end()) {
    size_t tmp = ith->second - ith->first;
    rh.erase(ith);
    return tmp;
  }
  return 0;
}

static bool memory_include(const memory_interval_t &m, const uintptr_t val) {
  // if m empty, begin == end
  const auto it = m.upper_bound(val);
  if (it == m.cbegin())
    return false;
  return (val < (std::prev(it)->second));
}

static const char *memory_location(data_t *state, hp_t hp, const uintptr_t ptr) {
  const auto &rs = state->rangeset_memory_shared[hp];
  if (memory_include(rs, ptr))
    return "S";

  const auto &rd = state->rangeset_memory_device[hp];
  if (memory_include(rd, ptr))
    return "D";

  const auto &rh = state->rangeset_memory_host[hp];
  if (memory_include(rh, ptr))
    return "H";

  return "M";
}

/*
 * Clock buiness
 */

static uint64_t convert_device_cycle(uint64_t device_cycle,
                                     const clock_lttng_device_t &timestamp_pair_ref,
                                     const uint64_t lttng_min,
                                     const ze_device_properties_t &device_property) {

  const auto &[lttng_ref, device_cycle_ref] = timestamp_pair_ref;

  assert(device_property.kernelTimestampValidBits <= 64);
  const uint64_t device_cycle_max_val =
      ((uint64_t)1 << device_property.kernelTimestampValidBits) - 1;

  uint64_t lttng;
  const uint64_t device_ref_ns =
      (device_cycle_ref & device_cycle_max_val) * device_property.timerResolution;
  device_cycle &= device_cycle_max_val;
  do {
    const uint64_t device_ns = device_cycle * device_property.timerResolution;
    lttng = device_ns + (lttng_ref - device_ref_ns);
    device_cycle += device_cycle_max_val;
  } while (lttng < lttng_min);
  return lttng;
}

static uint64_t compute_and_convert_delta(uint64_t start, uint64_t end,
                                          const ze_device_properties_t &device_property) {

  assert(device_property.kernelTimestampValidBits <= 64);
  const uint64_t max_val = ((uint64_t)1 << device_property.kernelTimestampValidBits) - 1;
  start &= max_val;
  end &= max_val;
  const uint64_t delta = (end >= start) ? (end - start) : (max_val + (end - start));
  return delta * device_property.timerResolution;
}

// Hash UUID
static uintptr_t hash_device(const ze_device_properties_t &device_property) {
  const auto device_uuid = device_property.uuid;

  // Right now we store `ptr` and not `uuid`.
  // we cast `uiid` to `ptr`. This should to the opposite
  uintptr_t device_uuid_hash[2];
  memcpy(&device_uuid_hash[0], &device_uuid, sizeof(device_uuid));
  device_uuid_hash[0] += device_uuid_hash[1];
  return device_uuid_hash[0];
}

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

/*
 * Host
 */

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_ts({hostname, vpid, vtid}, ts);
}

static void exits_callback(void *btx_handle, void *usr_data, int64_t ts,
                           const char *event_class_name, const char *hostname, int64_t vpid,
                           uint64_t vtid, ze_result_t zeResult) {
  auto *data = static_cast<data_t *>(usr_data);
  int64_t start = data->entry_state.get_ts({hostname, vpid, vtid});

  bool err = zeResult != ZE_RESULT_SUCCESS && zeResult != ZE_RESULT_NOT_READY;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start, BACKEND_ZE,
                              strip_event_class_name_exit(event_class_name).c_str(), (ts - start),
                              err);
}

/*
 * Device and Subdevice property
 */

static void property_device_callback(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *hostname, int64_t vpid, uint64_t vtid,
                                     ze_driver_handle_t hDriver, ze_device_handle_t hDevice,
                                     size_t _pDeviceProperties_val_length,
                                     ze_device_properties_t *pDeviceProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->device_property[{hostname, vpid, (thapi_device_id)hDevice}] = *pDeviceProperties_val;
}

static void property_subdevice_callback(void *btx_handle, void *usr_data, int64_t ts,
                                        const char *hostname, int64_t vpid, uint64_t vtid,
                                        ze_driver_handle_t hDriver, ze_device_handle_t hDevice,
                                        ze_device_handle_t hSubDevice,
                                        size_t _pDeviceProperties_val_length,
                                        ze_device_properties_t *pDeviceProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  hp_device_t hpd{hostname, vpid, (thapi_device_id)hSubDevice};
  data->device_property[hpd] = *pDeviceProperties_val;
  data->subdevice_parent[hpd] = (thapi_device_id)hDevice;
}

//
//   _   _  |/  _  ._ ._   _  |
//   /_ (/_ |\ (/_ |  | | (/_ |
//
static void property_kernel_callback(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *hostname, int64_t vpid, uint64_t vtid,
                                     ze_kernel_handle_t hKernel,
                                     size_t _pKernelProperties_val_length,
                                     ze_kernel_properties_t *pKernelProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  auto &a = data->kernelToDesct[{hostname, vpid, hKernel}];
  std::get<ze_kernel_properties_t>(a) = *pKernelProperties_val;
}

static void zeKernelCreate_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *event_class_name, const char *hostname,
                                          int64_t vpid, uint64_t vtid,
                                          char *desc__pKernelName_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, std::string{desc__pKernelName_val});
}

static void zeKernelCreate_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                         const char *event_class_name, const char *hostname,
                                         int64_t vpid, uint64_t vtid, ze_kernel_handle_t hKernel) {

  auto *data = static_cast<data_t *>(usr_data);
  // No need to check for Error, if not success, people should will not use it.
  const auto kernelName = data->entry_state.get_data<std::string>({hostname, vpid, vtid});
  auto &a = data->kernelToDesct[{hostname, vpid, hKernel}];
  std::get<std::string>(a) = kernelName;
}

// It's possible to bypass zeKernelCreate,
//      as a workaround for now, hoping that people will call
//      zeKernelGetName
static void zeKernelGetName_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *event_class_name, const char *hostname,
                                           int64_t vpid, uint64_t vtid, ze_kernel_handle_t hKernel,
                                           size_t pSize_val) {

  if (pSize_val == 0)
    return;
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hKernel);
}

static void zeKernelGetName_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *event_class_name, const char *hostname,
                                          int64_t vpid, uint64_t vtid, size_t _pName_val_length,
                                          char *pName_val) {

  if (_pName_val_length == 0)
    return;
  auto *data = static_cast<data_t *>(usr_data);
  const auto hKernel = data->entry_state.get_data<ze_kernel_handle_t>({hostname, vpid, vtid});
  auto &a = data->kernelToDesct[{hostname, vpid, hKernel}];
  std::get<std::string>(a) = pName_val;
}

// TODO: Need to handle error!
static void zeKernelSetGroupSize_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname, int64_t vpid, uint64_t vtid,
                                                ze_kernel_handle_t hKernel, uint32_t groupSizeX,
                                                uint32_t groupSizeY, uint32_t groupSizeZ) {

  auto *data = static_cast<data_t *>(usr_data);
  auto &a = data->kernelToDesct[{hostname, vpid, hKernel}];
  std::get<btx_kernel_group_size_t>(a) = {groupSizeX, groupSizeY, groupSizeZ};
}

/*           _                                 _                                    _
 *  _   _  /   _  ._ _  ._ _   _. ._   _|  / / \      _       _    |  o  _ _|_ \  /  ._ _   _. _|_ _
 *  /_ (/_ \_ (_) | | | | | | (_| | | (_| |  \_X |_| (/_ |_| (/_ o |_ | _>  |_  | \_ | (/_ (_|  |_
 * (/_ \                     /             /
 */
static void zeCommandListCreateImmediate_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_command_queue_desc_t *altdesc,
    ze_command_list_handle_t *phCommandList, size_t _altdesc_val_length,
    ze_command_queue_desc_t *altdesc_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->imm_tmp[{hostname, vpid, vtid}] = {hDevice, *altdesc_val};
}

static void zeCommandListCreateImmediate_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                       const char *hostname, int64_t vpid,
                                                       uint64_t vtid, ze_result_t zeResult,
                                                       ze_command_list_handle_t hCommandList) {
  auto *data = static_cast<data_t *>(usr_data);
  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  auto [hDevice, commandQueueDesc] = data->imm_tmp[{hostname, vpid, vtid}];
  data->commandListToBtxDesc[{hostname, vpid, hCommandList}] = {commandQueueDesc, hDevice, true};
}

static void zeCommandListCreate_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_command_list_desc_t *desc,
    ze_command_list_handle_t *phCommandList, size_t _desc_val_length,
    ze_command_list_desc_t *desc_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hDevice);
}

static void zeCommandListCreate_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                              const char *hostname, int64_t vpid, uint64_t vtid,
                                              ze_result_t zeResult,
                                              ze_command_list_handle_t hCommandList) {

  auto *data = static_cast<data_t *>(usr_data);
  const auto hDevice = data->entry_state.get_data<ze_device_handle_t>({hostname, vpid, vtid});
  if (zeResult != ZE_RESULT_SUCCESS)
    return;
  // ze_command_queue_desc_t  will be binded during CommandQueue Execute
  data->commandListToBtxDesc[{hostname, vpid, hCommandList}] = {ze_command_queue_desc_t{}, hDevice,
                                                                false};
}

static void zeCommandQueueCreate_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_command_queue_desc_t *desc,
    ze_command_queue_handle_t *phCommandQueue, size_t _desc_val_length,
    ze_command_queue_desc_t *desc_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, *desc_val);
}

static void zeCommandQueueCreate_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                               const char *hostname, int64_t vpid, uint64_t vtid,
                                               ze_result_t zeResult,
                                               ze_command_queue_handle_t hCommandQueue) {

  auto *data = static_cast<data_t *>(usr_data);
  const auto desc = data->entry_state.get_data<ze_command_queue_desc_t>({hostname, vpid, vtid});

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  data->commandQueueToDesc[{hostname, vpid, hCommandQueue}] = desc;
}

/*           _
 *   _   _  /   _  ._ _  ._ _   _. ._   _| |  o  _ _|_  /\  ._  ._   _  ._   _| \|/
 *   /_ (/_ \_ (_) | | | | | | (_| | | (_| |_ | _>  |_ /--\ |_) |_) (/_ | | (_| /|\
 */
static void hSignalEvent_hKernel_with_group_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel, ze_group_count_t *pLaunchFuncArgs_val) {

  auto *data = static_cast<data_t *>(usr_data);
  auto &a = data->kernelToDesct[{hostname, vpid, hKernel}];

  auto name = std::get<std::string>(a);
  std::string metadata;
  {
    auto &[groupSizeX, groupSizeY, groupSizeZ] = std::get<btx_kernel_group_size_t>(a);

    std::stringstream metadata_s;
    metadata_s << "SIMD" << std::get<ze_kernel_properties_t>(a).maxSubgroupSize << ", {"
               << pLaunchFuncArgs_val->groupCountX << "," << pLaunchFuncArgs_val->groupCountY << ","
               << pLaunchFuncArgs_val->groupCountZ << "}"
               << ", {" << groupSizeX << "," << groupSizeY << "," << groupSizeZ << "}";
    metadata = metadata_s.str();
  }
  data->threadToLastLaunchInfo[{hostname, vpid, vtid}] = {
      hCommandList, name, ts, btx_event_t::KERNEL, btx_additional_info_kernel_t{metadata}};
}

static void hSignalEvent_hKernel_without_group_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid, ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel) {

  auto *data = static_cast<data_t *>(usr_data);
  auto name = std::get<std::string>(data->kernelToDesct[{hostname, vpid, hKernel}]);
  data->threadToLastLaunchInfo[{hostname, vpid, vtid}] = {
      hCommandList, name, ts, btx_event_t::OTHER, {}};
}

static void hSignalEvent_eventMemory_2ptr_entry_callback(void *btx_handle, void *usr_data,
                                                         int64_t ts, const char *event_class_name,
                                                         const char *hostname, int64_t vpid,
                                                         uint64_t vtid,
                                                         ze_command_list_handle_t hCommandList,
                                                         void *dstptr, void *srcptr, size_t size) {

  auto *data = static_cast<data_t *>(usr_data);
  const hp_t hp{hostname, vpid};

  std::string name;
  {
    std::stringstream ss_name;
    ss_name << strip_event_class_name_entry(event_class_name) << "("
            << memory_location(data, hp, (uintptr_t)srcptr) << "2"
            << memory_location(data, hp, (uintptr_t)dstptr) << ")";
    name = ss_name.str();
  }
  data->threadToLastLaunchInfo[{hostname, vpid, vtid}] = {
      hCommandList, name, ts, btx_event_t::TRAFFIC, btx_additional_info_traffic_t{ts, size}};
}

static void hSignalEvent_eventMemory_1ptr_entry_callback(void *btx_handle, void *usr_data,
                                                         int64_t ts, const char *event_class_name,
                                                         const char *hostname, int64_t vpid,
                                                         uint64_t vtid,
                                                         ze_command_list_handle_t hCommandList,
                                                         void *ptr, size_t size) {

  auto *data = static_cast<data_t *>(usr_data);
  const hp_t hp{hostname, vpid};

  std::string name;
  {
    std::stringstream name_s;
    name_s << strip_event_class_name_entry(event_class_name) << "("
           << memory_location(data, hp, (uintptr_t)ptr) << ")";
    name = name_s.str();
  }
  data->threadToLastLaunchInfo[{hostname, vpid, vtid}] = {
      hCommandList, name, ts, btx_event_t::TRAFFIC, btx_additional_info_traffic_t{ts, size}};
}

static void eventMemory_without_hSignalEvent_entry_callback(void *btx_handle, void *usr_data,
                                                            int64_t ts,
                                                            const char *event_class_name,
                                                            const char *hostname, int64_t vpid,
                                                            uint64_t vtid, size_t size) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, size);
}

static void eventMemory_without_hSignalEvent_exit_callback(void *btx_handle, void *usr_data,
                                                           int64_t ts, const char *event_class_name,
                                                           const char *hostname, int64_t vpid,
                                                           uint64_t vtid, ze_result_t ze_result) {
  auto *data = static_cast<data_t *>(usr_data);
  auto size = data->entry_state.get_data<size_t>({hostname, vpid, vtid});

  if (ze_result != ZE_RESULT_SUCCESS)
    return;
  int64_t start = data->entry_state.get_ts({hostname, vpid, vtid});
  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, start, BACKEND_ZE,
                                 strip_event_class_name_exit(event_class_name).c_str(), size, "");
}

// Barrier
static void hSignalEvent_rest_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *event_class_name, const char *hostname,
                                             int64_t vpid, uint64_t vtid,
                                             ze_command_list_handle_t hCommandList) {

  auto *data = static_cast<data_t *>(usr_data);
  std::string name = strip_event_class_name_entry(event_class_name);
  data->threadToLastLaunchInfo[{hostname, vpid, vtid}] = {
      hCommandList, name, ts, btx_event_t::OTHER, {}};
}

/*
 *             _                              _                   _
 *     _   _  /   _  ._ _  ._ _   _. ._   _| / \      _       _  |_     _   _    _|_  _
 *     /_ (/_ \_ (_) | | | | | | (_| | | (_| \_X |_| (/_ |_| (/_ |_ >< (/_ (_ |_| |_ (/_
 *
 */
static void zeCommandQueueExecuteCommandLists_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists,
    ze_command_list_handle_t *phCommandLists, ze_fence_handle_t hFence,
    size_t _phCommandLists_vals_length, ze_command_list_handle_t *phCommandLists_vals) {

  auto *data = static_cast<data_t *>(usr_data);

  const auto commandQueueDesc = data->commandQueueToDesc[{hostname, vpid, hCommandQueue}];
  for (size_t i = 0; i < _phCommandLists_vals_length; i++) {
    for (auto &hEvent : data->commandListToEvents[{hostname, vpid, phCommandLists_vals[i]}]) {
      auto &h = data->eventToBtxDesct[{hostname, vpid, hEvent}];
      std::get<ze_command_queue_desc_t>(h) = commandQueueDesc;
      std::get<int64_t>(h) = ts;
    }
  }
}

/*    _                         __                         _
 *   |_) _|_    |\/|  _  ._ _  /__  _ _|_  /\  | |  _   _ |_) ._ _  ._   _  ._ _|_ o  _   _
 *   |_)  |_ >< |  | (/_ | | | \_| (/_ |_ /--\ | | (_) (_ |   | (_) |_) (/_ |   |_ | (/_ _>
 *                                                                  |
 */
// zeModuleGetGlobalPointer and zeModuleDestroy
static void zeModule_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                    const char *event_class_name, const char *hostname,
                                    int64_t vpid, uint64_t vtid, ze_module_handle_t hModule) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hModule);
}

static void zeModuleGetGlobalPointer_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                   const char *hostname, int64_t vpid,
                                                   uint64_t vtid, ze_result_t zeResult,
                                                   size_t pSize_val, void *pptr_val) {

  auto *data = static_cast<data_t *>(usr_data);
  auto hModule = data->entry_state.get_data<ze_module_handle_t>({hostname, vpid, vtid});

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  // Used to free memory
  data->module_global_pointer[{hostname, vpid, hModule}].insert((uintptr_t)pptr_val);
  // Lets consider it like shared memory
  add_memory(data, {hostname, vpid}, (uintptr_t)pptr_val, pSize_val,
             "lttng_ust_ze:zeMemAllocShared_exit");
}

static void zeModuleDestroy_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *hostname, int64_t vpid, uint64_t vtid,
                                          ze_result_t zeResult) {
  auto *data = static_cast<data_t *>(usr_data);
  auto hModule = data->entry_state.get_data<ze_module_handle_t>({hostname, vpid, vtid});

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  auto &s = data->module_global_pointer;
  auto it = s.find({hostname, vpid, hModule});
  // This should never happen
  if (it == s.end())
    return;

  for (auto &s2 : it->second)
    data->rangeset_memory_shared[{hostname, vpid}].erase(s2);
  s.erase(it);
}

static void entries_alloc_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name, const char *hostname, int64_t vpid,
                                   uint64_t vtid, size_t size) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, size);
}

static void exits_alloc_callback(void *btx_handle, void *usr_data, int64_t ts,
                                 const char *event_class_name, const char *hostname, int64_t vpid,
                                 uint64_t vtid, ze_result_t zeResult, void *pptr_val) {
  auto *data = static_cast<data_t *>(usr_data);
  if (zeResult == ZE_RESULT_SUCCESS) {
    auto size = data->entry_state.get_data<size_t>({hostname, vpid, vtid});
    add_memory(data, {hostname, vpid}, (uintptr_t)pptr_val, size, event_class_name);
  }
}

static void memory_info_properties_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ze_context_handle_t hContext, void *ptr, size_t _pMemAllocProperties_val_length,
    ze_memory_allocation_properties_t *pMemAllocProperties_val, ze_device_handle_t hDevice) {

  auto *data = static_cast<data_t *>(usr_data);

  std::unordered_map<hp_t, memory_interval_t> *mi = nullptr;
  if (pMemAllocProperties_val->type == ZE_MEMORY_TYPE_HOST)
    mi = &data->rangeset_memory_host;
  else if (pMemAllocProperties_val->type == ZE_MEMORY_TYPE_DEVICE)
    mi = &data->rangeset_memory_device;
  else if (pMemAllocProperties_val->type == ZE_MEMORY_TYPE_SHARED)
    mi = &data->rangeset_memory_shared;
  else if (pMemAllocProperties_val->type == ZE_MEMORY_TYPE_UNKNOWN)
    return;
  else {
    std::cout << "WARNING Unknow memory_info_properties: " << pMemAllocProperties_val->type
              << std::endl;
    return;
  }
  data->rangeset_tmp[{hostname, vpid, vtid}] = mi;
}

static void memory_info_range_callback(void *btx_handle, void *usr_data, int64_t ts,
                                       const char *hostname, int64_t vpid, uint64_t vtid,
                                       ze_context_handle_t hContext, void *ptr, void *base,
                                       size_t size) {

  auto *data = static_cast<data_t *>(usr_data);
  auto *mi = data->rangeset_tmp[{hostname, vpid, vtid}];
  (*mi)[{hostname, vpid}][(uintptr_t)base] = (uintptr_t)base + size;
}
/*
 * Remove Memory
 */
static void memFree_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name, const char *hostname, int64_t vpid,
                                   uint64_t vtid, void *ptr) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, ptr);
}

static void memFree_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                  const char *event_class_name, const char *hostname, int64_t vpid,
                                  uint64_t vtid, ze_result_t ze_result) {

  auto *data = static_cast<data_t *>(usr_data);
  if (ze_result == ZE_RESULT_SUCCESS) {
    auto ptr = data->entry_state.get_data<uintptr_t>({hostname, vpid, vtid});
    remove_memory(data, {hostname, vpid}, ptr);
  }
}

/*    _          _
 *   |_) _|_    /  |  _   _ |
 *   |_)  |_ >< \_ | (_) (_ |<
 */
static void property_device_timer_callback(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *hostname, int64_t vpid, uint64_t vtid,
                                           ze_device_handle_t hDevice, uint64_t hostTimestamp,
                                           uint64_t deviceTimestamp) {
  auto *data = static_cast<data_t *>(usr_data);
  data->device_timestamps_pair_ref[{hostname, vpid, (thapi_device_id)hDevice}] = {ts,
                                                                                  deviceTimestamp};
}

/*
 *      _         _                  _
 *     |_) ._ _ _|_ o | o ._   _    |_     _  ._ _|_
 *     |   | (_) |  | | | | | (_|   |_ \/ (/_ | | |_
 *                             _|
 */
static void event_profiling_callback(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *hostname, int64_t vpid, uint64_t vtid,
                                     ze_event_handle_t hEvent) {

  auto *data = static_cast<data_t *>(usr_data);

  auto it_pp = data->threadToLastLaunchInfo.find({hostname, vpid, vtid});
  // We didn't find the command who initiated this even_profiling,
  // This mean we should ignore it, Either mean nothing, instaneous
  // or not supported
  //
  // zeCommandListAppendSignalEvent|zeCommandListAppendLaunchMultipleKernelsIndirect...
  //
  if (it_pp == data->threadToLastLaunchInfo.end())
    return;

  // Don't put an `&` is break the metadata / commandName... Not sure why
  const auto [hCommandList, commandName, ts_min, type, ptr] = it_pp->second;
  data->threadToLastLaunchInfo.erase(it_pp);

  auto [commandQueueDesc, hDevice, hCommandListIsImmediate] =
      data->commandListToBtxDesc[{hostname, vpid, hCommandList}];

  // Got the timestamp pair reference
  clock_lttng_device_t clockLttngDevice;
  {
    const auto &m0 = data->device_timestamps_pair_ref;
    const auto it0 = m0.find({hostname, vpid, (thapi_device_id)hDevice});
    if (it0 != m0.cend())
      clockLttngDevice = it0->second;
  }

  // If not IMM will be commandQueueDesc overwrited latter
  data->eventToBtxDesct[{hostname, vpid, hEvent}] = {vtid,         commandQueueDesc,
                                                     hCommandList, hCommandListIsImmediate,
                                                     hDevice,      commandName,
                                                     ts_min,       clockLttngDevice,
                                                     type,         ptr};
  // Prepare job for non IMM
  if (!hCommandListIsImmediate)
    data->commandListToEvents[{hostname, vpid, hCommandList}].insert(hEvent);
}

static void zeCommandListReset_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                              const char *hostname, int64_t vpid, uint64_t vtid,
                                              ze_command_list_handle_t hCommandList) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hCommandList);
}

static void zeCommandListReset_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *hostname, int64_t vpid, uint64_t vtid,
                                             ze_result_t zeResult) {

  auto *data = static_cast<data_t *>(usr_data);
  if (zeResult == ZE_RESULT_SUCCESS) {
    auto hCommandList =
        data->entry_state.get_data<ze_command_list_handle_t>({hostname, vpid, vtid});
    data->commandListToEvents[{hostname, vpid, hCommandList}].clear();
  }
}

static void event_profiling_result_callback(void *btx_handle, void *usr_data, int64_t ts,
                                            const char *hostname, int64_t vpid, uint64_t vtid,
                                            ze_event_handle_t hEvent, ze_result_t status,
                                            ze_result_t timestampStatus, uint64_t globalStart,
                                            uint64_t globalEnd, uint64_t contextStart,
                                            uint64_t contextEnd) {

  if (status == ZE_RESULT_NOT_READY)
    return;

  auto *data = static_cast<data_t *>(usr_data);

  // TODO: Should  we always find the eventToBtxDesct?
  // We didn't find the partial payload, that mean we should ignore it
  const auto it_p = data->eventToBtxDesct.find({hostname, vpid, hEvent});
  if (it_p == data->eventToBtxDesct.cend())
    return;
  // We don't erase, may have one entry for multiple result
  const auto &[vtid_submission, commandQueueDesc, hCommandList, hCommandListIsImmediate, device,
               commandName, lltngMin, clockLttngDevice, type, ptr] = it_p->second;
  std::string metadata = "";
  {
    std::stringstream ss_metadata;
    if ((type == btx_event_t::KERNEL) && (status == ZE_RESULT_SUCCESS))
      ss_metadata << std::get<btx_additional_info_kernel_t>(ptr) << ", ";
    // Create additional Medatata of the Command Queue
    ss_metadata << "{ordinal: " << commandQueueDesc.ordinal << ", "
                << "index: " << commandQueueDesc.index << "}";
    metadata = ss_metadata.str();
  }
  if (!hCommandListIsImmediate)
    data->commandListToEvents[{hostname, vpid, hCommandList}].erase(hEvent);

  if ((type == btx_event_t::TRAFFIC) && (status == ZE_RESULT_SUCCESS)) {
    auto &[ts, size] = std::get<btx_additional_info_traffic_t>(ptr);
    btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, ts, BACKEND_ZE,
                                   commandName.c_str(), size, metadata.c_str());
  }
  const bool err = ((status != ZE_RESULT_SUCCESS) || (timestampStatus != ZE_RESULT_SUCCESS));
  // No device information. No conversion to ns, no looping
  uint64_t delta = globalEnd - globalStart;
  uint64_t start = lltngMin;
  uintptr_t device_hash = 0;
  const auto it0 = data->device_property.find({hostname, vpid, (thapi_device_id)device});
  if (it0 != data->device_property.cend()) {
    if (!err) {
      delta = compute_and_convert_delta(globalStart, globalEnd, it0->second);
      start = convert_device_cycle(globalStart, clockLttngDevice, lltngMin, it0->second);
    }
    device_hash = hash_device(it0->second);
  }
  uintptr_t subdevice_hash = 0;
  const auto it1 = data->subdevice_parent.find({hostname, vpid, (thapi_device_id)device});
  if (it1 != data->subdevice_parent.cend()) {
    subdevice_hash = device_hash;
    const auto it2 = data->device_property.find({hostname, vpid, it1->second});
    if (it2 != data->device_property.cend())
      subdevice_hash = hash_device(it2->second);
  }

  btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid_submission, start, BACKEND_ZE,
                                commandName.c_str(), delta, device_hash, subdevice_hash, err,
                                metadata.c_str());
}

static void zeEventDestroy_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *hostname, int64_t vpid, uint64_t vtid,
                                          ze_event_handle_t hEvent) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hEvent);
}

static void zeEventDestroy_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                         const char *hostname, int64_t vpid, uint64_t vtid,
                                         ze_result_t zeResult) {

  auto *data = static_cast<data_t *>(usr_data);
  auto hEvent = data->entry_state.get_data<ze_event_handle_t>({hostname, vpid, vtid});

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  data->eventToBtxDesct.erase({hostname, vpid, hEvent});
}

/*
 * Sampling
 */

static void lttng_ust_ze_sampling_fabricPort_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                      const char *hostname, int64_t vpid,
                                                      uint64_t vtid, ze_device_handle_t hDevice,
                                                      zes_fabric_port_handle_t hFabricPort,
                                                      size_t _pFabricPortState_val_length,
                                                      zes_fabric_port_state_t *pFabricPortState_val,
                                                      size_t _pFabricPortThroughput_val_length,
                                                      zes_fabric_port_throughput_t *pFabricPortThroughput_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto it0 = data->fabricPort_property.find({hostname, vpid, hDevice, hFabricPort});
  if (it0 != data->fabricPort_property.cend()) {
    // Get fabricPort properties: subdevice ID, fabeicId ...
    auto subDevice = it0->second.subdeviceId;
    auto fabricId = it0->second.portId.fabricId;
    auto remotePortId = pFabricPortState_val->remotePortId.fabricId;
    // Current Speed 
    double rxSpeed = static_cast<double>(pFabricPortState_val->rxSpeed.bitRate * pFabricPortState_val->rxSpeed.width)/8.0;
    double txSpeed = static_cast<double>(pFabricPortState_val->txSpeed.bitRate * pFabricPortState_val->txSpeed.width)/8.0;
   
    // Insert the current throughput data with timestamp
    auto [it, inserted] = data->device_fabricPort_ref.insert(
            {{hostname, vpid, hDevice, hFabricPort, subDevice}, {*pFabricPortThroughput_val, ts}});
    if (inserted)
      return;

    // Previous throughput data
    auto &[prev_throughput, prev_ts] = it->second;
    
    /* Per doc: When taking the delta, the difference between timestamp samples
    * could be 0, if the frequency of sampling the snapshots is higher than the
    * frequency of the timestamp update. */
    if (pFabricPortThroughput_val->timestamp == prev_throughput.timestamp)
      return;

    double time_diff = static_cast<double>(pFabricPortThroughput_val->timestamp - prev_throughput.timestamp);
    // Calculate the RX and TX throughput 
    double rxThroughput = static_cast<double>(pFabricPortThroughput_val->rxCounter - prev_throughput.rxCounter) / double(time_diff);
    double txThroughput = static_cast<double>(pFabricPortThroughput_val->txCounter - prev_throughput.txCounter) / double(time_diff);

    btx_push_message_lttng_fabricPort(btx_handle, hostname, 0, 0, prev_ts, BACKEND_ZE, 
                                      (uint64_t)hDevice,(uint64_t)hFabricPort, subDevice, 
                                      fabricId, remotePortId, rxThroughput, txThroughput, 
                                      rxSpeed, txSpeed);
    // Update the stored values
    it->second = {*pFabricPortThroughput_val, ts};
    } else {
      std::cerr << "Fabricport property not found!" << std::endl;
    }
}

static void lttng_ust_ze_sampling_engineStats_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                       const char *hostname, int64_t vpid,
                                                       uint64_t vtid, ze_device_handle_t hDevice,
                                                       zes_engine_handle_t hEngine,
                                                       size_t _pEngineStats_val_length,
                                                       zes_engine_stats_t *pEngineStats_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto it0 = data->engine_property.find({hostname, vpid, hDevice, hEngine});
  if (it0 != data->engine_property.cend()) {
    const auto& engineProps = it0->second;
    uint32_t subDevice = (engineProps.onSubdevice) ? engineProps.subdeviceId : 0;

    if (engineProps.type == ZES_ENGINE_GROUP_COMPUTE_ALL || engineProps.type == ZES_ENGINE_GROUP_COPY_ALL) {
      auto [it, inserted] = data->device_engines_ref.insert(
            {{hostname, vpid, hDevice, hEngine, subDevice}, {*pEngineStats_val, ts}});
      if (inserted)
        return;
      auto &[prev_engineStats, prev_ts] = it->second;
      if (pEngineStats_val->timestamp == prev_engineStats.timestamp)
        return;

      double time_diff = static_cast<double>(pEngineStats_val->timestamp - prev_engineStats.timestamp);
      double activeTime = static_cast<double>(pEngineStats_val->activeTime - prev_engineStats.activeTime) / time_diff;

      if (engineProps.type == ZES_ENGINE_GROUP_COMPUTE_ALL) {
        btx_push_message_lttng_computeEU(btx_handle, hostname, 0, 0, prev_ts, BACKEND_ZE,
                                        (uint64_t)hDevice,  subDevice, activeTime);
      } else {
        btx_push_message_lttng_copyEU(btx_handle, hostname,0, 0, prev_ts, BACKEND_ZE,
                                     (uint64_t)hDevice, subDevice, activeTime);
      }
      it->second = {*pEngineStats_val, ts};
    } 
  } else {
    std::cerr << "Engine property not found for device: " << hDevice << std::endl;
  }
}
   
static void lttng_ust_ze_sampling_gpu_energy_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                      const char *hostname, int64_t vpid,
                                                      uint64_t vtid, ze_device_handle_t hDevice,
                                                      zes_pwr_handle_t hPower,
                                                      size_t _pEnergyCounter_val_length,
                                                      zes_power_energy_counter_t *pEnergyCounter_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto it0 = data->power_property.find({hostname, vpid, hDevice, hPower});
  if (it0 != data->power_property.cend()) {
    const auto& powerProps = it0->second;
    uint32_t domain = (powerProps.onSubdevice) ? powerProps.subdeviceId + 1 : 0;
    auto [it, inserted] = data->device_energy_ref.insert(
            {{hostname, vpid, hDevice, hPower, domain}, {*pEnergyCounter_val, ts}});
    if (inserted)
      return;

    auto &[prev_EnergyCounter, prev_ts] = it->second;
    if (pEnergyCounter_val->timestamp == prev_EnergyCounter.timestamp)
      return;

    double time_diff = static_cast<double>(pEnergyCounter_val->timestamp - prev_EnergyCounter.timestamp);
    double power = static_cast<double>(pEnergyCounter_val->energy - prev_EnergyCounter.energy) / time_diff;
    btx_push_message_lttng_power(btx_handle, hostname, 0, 0, prev_ts, BACKEND_ZE, (uint64_t)hDevice,
                               (thapi_domain_idx)domain, power);
    it->second = {*pEnergyCounter_val, ts};
  } else {
    std::cerr << "Power property not found for device: " << hDevice << std::endl;
  }
}


static void lttng_ust_ze_sampling_gpu_frequency_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                         const char *hostname, int64_t vpid,
                                                         uint64_t vtid, ze_device_handle_t hDevice,
                                                         zes_freq_handle_t hFrequency,
                                                         size_t _pFreqState_val_length,
                                                         zes_freq_state_t *pFreqState_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto it0 = data->frequency_property.find({hostname, vpid, hDevice, hFrequency});
  if (it0 != data->frequency_property.cend()) {
    const auto& freqProps = it0->second;
    uint32_t domain = (freqProps.onSubdevice) ? freqProps.subdeviceId : 0;
    
    btx_push_message_lttng_frequency(btx_handle, hostname, 0, 0, ts, BACKEND_ZE, (uint64_t)hDevice,
                               (thapi_domain_idx)domain, pFreqState_val->actual);
  }else {
   std::cerr << "Frequency property not found for device: " << hDevice << std::endl;
  }
}
// Properties
static void lttng_ust_ze_sampling_fabricPortProperties_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                                const char *hostname, int64_t vpid, uint64_t vtid,
                                                                ze_device_handle_t hDevice, zes_fabric_port_handle_t hFabricPort,
                                                                size_t _pFabricPortProperties_val_length,
                                                                zes_fabric_port_properties_t *pFabricPortProperties_val) {
  auto *data = static_cast<data_t *>(usr_data);
  data->fabricPort_property[{hostname, vpid, (ze_device_handle_t)hDevice, (zes_fabric_port_handle_t)hFabricPort}] = *pFabricPortProperties_val;
} 

static void lttng_ust_ze_sampling_powerProperties_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                           const char *hostname, int64_t vpid, uint64_t vtid,
                                                           ze_device_handle_t hDevice, zes_pwr_handle_t hPower,
                                                           size_t _pPowerProperties_val_length,
                                                           zes_power_properties_t *pPowerProperties_val) {
  auto *data = static_cast<data_t *>(usr_data);
  data->power_property[{hostname, vpid, (ze_device_handle_t)hDevice, (zes_pwr_handle_t)hPower}] = *pPowerProperties_val;
}

static void lttng_ust_ze_sampling_freqProperties_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                          const char *hostname, int64_t vpid, uint64_t vtid,
                                                          ze_device_handle_t hDevice, zes_freq_handle_t hFrequency,
                                                          size_t _pfreqProperties_val_length,
                                                          zes_freq_properties_t *pFreqProperties_val) {
  auto *data = static_cast<data_t *>(usr_data);
  data->frequency_property[{hostname, vpid, (ze_device_handle_t)hDevice, (zes_freq_handle_t)hFrequency}] = *pFreqProperties_val;
}

static void lttng_ust_ze_sampling_engineProperties_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                            const char *hostname, int64_t vpid, uint64_t vtid,
                                                            ze_device_handle_t hDevice, zes_engine_handle_t hEngine,
                                                            size_t _pEngineProperties_val_length,
                                                            zes_engine_properties_t *pEngineProperties_val) {
  auto *data = static_cast<data_t *>(usr_data);
  data->engine_property[{hostname, vpid, (ze_device_handle_t)hDevice, (zes_engine_handle_t)hEngine}] = *pEngineProperties_val;
}

static void lttng_ust_ze_sampling_fabricPort_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                      const char *hostname, int64_t vpid,
                                                      uint64_t vtid, ze_device_handle_t hDevice,
                                                      zes_fabric_port_handle_t hFabricPort,
                                                      size_t _pFabricPortState_val_length,
                                                      zes_fabric_port_state_t *pFabricPortState_val,
                                                      size_t _pFabricPortThroughput_val_length,
                                                      zes_fabric_port_throughput_t *pFabricPortThroughput_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto it0 = data->fabricPort_property.find({hostname, vpid, hDevice, hFabricPort});
  if (it0 != data->fabricPort_property.cend()) {
    //Get the subdevice ID
    auto subDevice = it0->second.subdeviceId;
    // Insert the current throughput data with timestamp
    auto [it, inserted] = data->device_fabricPort_ref.insert(
            {{hostname, vpid, hDevice, hFabricPort, subDevice}, {*pFabricPortThroughput_val, ts}});
    if (inserted)
      return;

    //  previous throughput data
    auto &[prev_throughput, prev_ts] = it->second;
    //  time difference
    double time_diff = static_cast<double>(pFabricPortThroughput_val->timestamp - prev_throughput.timestamp);
    // Calculate the RX and TX speeds
    double rxSpeed = static_cast<double>(pFabricPortThroughput_val->rxCounter - prev_throughput.rxCounter) / double(time_diff);
    double txSpeed = static_cast<double>(pFabricPortThroughput_val->txCounter - prev_throughput.txCounter) / double(time_diff);

    // Update the stored values
    it->second = {*pFabricPortThroughput_val, ts};
    btx_push_message_lttng_fabricPort(btx_handle, hostname, 0, 0, prev_ts,
                                      BACKEND_ZE, (uint64_t)hDevice,(uint64_t)hFabricPort,
                                      subDevice, rxSpeed, txSpeed);
    } else {
      std::cerr << "Fabric port property not found!" << std::endl;
    }

}

// fabricPort Property
static void lttng_ust_ze_sampling_fabricPortProperties_callback(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *hostname, int64_t vpid, uint64_t vtid,
                                     ze_device_handle_t hDevice, zes_fabric_port_handle_t hFabricPort,
                                     size_t _pFabricPortProperties_val_length,
                                     zes_fabric_port_properties_t *pFabricPortProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->fabricPort_property[{hostname, vpid, (ze_device_handle_t)hDevice, (zes_fabric_port_handle_t)hFabricPort}] = *pFabricPortProperties_val;
} 

/*
 * Register
 */
#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                                    \
  btx_register_callbacks_##base_name(btx_handle, &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  /* Host*/
  REGISTER_ASSOCIATED_CALLBACK(entries);
  REGISTER_ASSOCIATED_CALLBACK(exits);

  /* Memory */
  REGISTER_ASSOCIATED_CALLBACK(entries_alloc);
  REGISTER_ASSOCIATED_CALLBACK(exits_alloc);
  REGISTER_ASSOCIATED_CALLBACK(zeModule_entry);
  btx_register_callbacks_lttng_ust_ze_properties_memory_info_properties(
      btx_handle, &memory_info_properties_callback);
  btx_register_callbacks_lttng_ust_ze_properties_memory_info_range(btx_handle,
                                                                   &memory_info_range_callback);

  /* Device and Subdevice property */
  btx_register_callbacks_lttng_ust_ze_properties_device(btx_handle, &property_device_callback);
  btx_register_callbacks_lttng_ust_ze_properties_subdevice(btx_handle,
                                                           &property_subdevice_callback);

  /* Map command list to device and to command queue dist*/
  btx_register_callbacks_lttng_ust_ze_zeCommandListCreateImmediate_entry(
      btx_handle, zeCommandListCreateImmediate_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandListCreateImmediate_exit(
      btx_handle, zeCommandListCreateImmediate_exit_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandListCreate_entry(btx_handle,
                                                                zeCommandListCreate_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandListCreate_exit(btx_handle,
                                                               zeCommandListCreate_exit_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandQueueCreate_entry(
      btx_handle, zeCommandQueueCreate_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandQueueCreate_exit(btx_handle,
                                                                zeCommandQueueCreate_exit_callback);

  btx_register_callbacks_lttng_ust_ze_zeCommandQueueExecuteCommandLists_entry(
      btx_handle, zeCommandQueueExecuteCommandLists_entry_callback);

  /*  Name of the Function Profiled  */
  REGISTER_ASSOCIATED_CALLBACK(zeKernelCreate_entry);
  REGISTER_ASSOCIATED_CALLBACK(zeKernelCreate_exit);
  REGISTER_ASSOCIATED_CALLBACK(zeKernelGetName_entry);
  REGISTER_ASSOCIATED_CALLBACK(zeKernelGetName_exit);

  btx_register_callbacks_lttng_ust_ze_zeKernelSetGroupSize_entry(
      btx_handle, &zeKernelSetGroupSize_entry_callback);

  btx_register_callbacks_lttng_ust_ze_properties_kernel(btx_handle, &property_kernel_callback);

  /* Drift */
  btx_register_callbacks_lttng_ust_ze_properties_device_timer(btx_handle,
                                                              &property_device_timer_callback);

  /* Profiling Command (everything who signal an event on completion)
   */
  REGISTER_ASSOCIATED_CALLBACK(hSignalEvent_hKernel_with_group_entry);
  REGISTER_ASSOCIATED_CALLBACK(hSignalEvent_hKernel_without_group_entry);

  REGISTER_ASSOCIATED_CALLBACK(hSignalEvent_eventMemory_2ptr_entry);
  REGISTER_ASSOCIATED_CALLBACK(hSignalEvent_eventMemory_1ptr_entry);

  REGISTER_ASSOCIATED_CALLBACK(eventMemory_without_hSignalEvent_entry);
  REGISTER_ASSOCIATED_CALLBACK(eventMemory_without_hSignalEvent_exit);
  REGISTER_ASSOCIATED_CALLBACK(hSignalEvent_rest_entry);

  /* Remove Memory */
  REGISTER_ASSOCIATED_CALLBACK(memFree_entry);
  REGISTER_ASSOCIATED_CALLBACK(memFree_exit);

  btx_register_callbacks_lttng_ust_ze_zeModuleGetGlobalPointer_exit(
      btx_handle, &zeModuleGetGlobalPointer_exit_callback);
  btx_register_callbacks_lttng_ust_ze_zeModuleDestroy_exit(btx_handle,
                                                           &zeModuleDestroy_exit_callback);

  /* Handling of event */
  btx_register_callbacks_lttng_ust_ze_profiling_event_profiling(btx_handle,
                                                                &event_profiling_callback);
  btx_register_callbacks_lttng_ust_ze_profiling_event_profiling_results(
      btx_handle, &event_profiling_result_callback);

  btx_register_callbacks_lttng_ust_ze_zeEventDestroy_entry(btx_handle,
                                                           &zeEventDestroy_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeEventDestroy_exit(btx_handle,
                                                          &zeEventDestroy_exit_callback);

  btx_register_callbacks_lttng_ust_ze_zeCommandListReset_entry(btx_handle,
                                                               &zeCommandListReset_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeCommandListReset_exit(btx_handle,
                                                              &zeCommandListReset_exit_callback);

  /* Sampling */
  btx_register_callbacks_lttng_ust_ze_sampling_fabricPortProperties(
      btx_handle, &lttng_ust_ze_sampling_fabricPortProperties_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_powerProperties(
      btx_handle, &lttng_ust_ze_sampling_powerProperties_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_engineProperties(
      btx_handle, &lttng_ust_ze_sampling_engineProperties_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_freqProperties(
      btx_handle, &lttng_ust_ze_sampling_freqProperties_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_fabricPort(
      btx_handle, &lttng_ust_ze_sampling_fabricPort_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_gpu_energy(
      btx_handle, &lttng_ust_ze_sampling_gpu_energy_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_gpu_frequency(
      btx_handle, &lttng_ust_ze_sampling_gpu_frequency_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_engineStats(
      btx_handle, &lttng_ust_ze_sampling_engineStats_callback);
}
