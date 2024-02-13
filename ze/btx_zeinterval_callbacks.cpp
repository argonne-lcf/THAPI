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

static void add_memory(data_t *state, hp_t hp, uintptr_t ptr, size_t size,
                       std::string source) {
  std::unordered_map<hp_t, memory_interval_t> *mi = nullptr;
  if (source == "lttng_ust_ze:zeMemAllocHost_exit")
    mi = &state->rangeset_memory_host;
  else if (source == "lttng_ust_ze:zeMemAllocDevice_exit")
    mi = &state->rangeset_memory_device;
  else if (source == "lttng_ust_ze:zeMemAllocShared_exit")
    mi = &state->rangeset_memory_shared;
  else
    std::cout << "WARNING Adding unknow memory " << source << "ptr " << ptr
              << std::endl;

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

static const char *memory_location(data_t *state, hp_t hp,
                                   const uintptr_t ptr) {
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

static uint64_t convert_device_cycle(
    uint64_t device_cycle, const clock_lttng_device_t &timestamp_pair_ref,
    const uint64_t lttng_min, const ze_device_properties_t &device_property) {

  const auto &[lttng_ref, device_cycle_ref] = timestamp_pair_ref;

  assert(device_property.kernelTimestampValidBits <= 64);
  const uint64_t device_cycle_max_val =
      ((uint64_t)1 << device_property.kernelTimestampValidBits) - 1;

  uint64_t lttng;
  const uint64_t device_ref_ns = (device_cycle_ref & device_cycle_max_val) *
                                 device_property.timerResolution;
  device_cycle &= device_cycle_max_val;
  do {
    const uint64_t device_ns = device_cycle * device_property.timerResolution;
    lttng = device_ns + (lttng_ref - device_ref_ns);
    device_cycle += device_cycle_max_val;
  } while (lttng < lttng_min);
  return lttng;
}

static uint64_t
compute_and_convert_delta(uint64_t start, uint64_t end,
                          const ze_device_properties_t &device_property) {

  assert(device_property.kernelTimestampValidBits <= 64);
  const uint64_t max_val =
      ((uint64_t)1 << device_property.kernelTimestampValidBits) - 1;
  start &= max_val;
  end &= max_val;
  const uint64_t delta =
      (end >= start) ? (end - start) : (max_val + (end - start));
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

static void btx_initialize_component(void **usr_data) {
  *usr_data = new data_t;
}

static void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

/*
 * Host
 */

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname,
                             int64_t vpid, uint64_t vtid) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_ts({hostname, vpid, vtid}, ts);
}

static void exits_callback(void *btx_handle, void *usr_data, int64_t ts,
                           const char *event_class_name, const char *hostname,
                           int64_t vpid, uint64_t vtid, ze_result_t zeResult) {
  auto *data = static_cast<data_t *>(usr_data);
  int64_t start = data->entry_state.get_ts({hostname, vpid, vtid});

  bool err = zeResult != ZE_RESULT_SUCCESS && zeResult != ZE_RESULT_NOT_READY;
  btx_push_message_lttng_host(
      btx_handle, hostname, vpid, vtid, start, BACKEND_ZE,
      strip_event_class_name_exit(event_class_name).c_str(), (ts - start), err);
}

static void entries_alloc_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname, int64_t vpid,
                                   uint64_t vtid, size_t size) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, size);
}

static void exits_alloc_callback(void *btx_handle, void *usr_data, int64_t ts,
                                 const char *event_class_name,
                                 const char *hostname, int64_t vpid,
                                 uint64_t vtid, ze_result_t zeResult,
                                 void *pptr_val) {
  auto *data = static_cast<data_t *>(usr_data);
  auto size = data->entry_state.get_data<size_t>({hostname, vpid, vtid});
  if (zeResult == ZE_RESULT_SUCCESS)
    add_memory(data, {hostname, vpid}, (uintptr_t)pptr_val, size,
               event_class_name);
}

/*
 * Device and Subdevice property
 */

static void
property_device_callback(void *btx_handle, void *usr_data, int64_t ts,
                         const char *hostname, int64_t vpid, uint64_t vtid,
                         ze_driver_handle_t hDriver, ze_device_handle_t hDevice,
                         size_t _pDeviceProperties_val_length,
                         ze_device_properties_t *pDeviceProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->device_property[{hostname, vpid, (thapi_device_id)hDevice}] =
      *pDeviceProperties_val;
}

static void property_subdevice_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_driver_handle_t hDriver,
    ze_device_handle_t hDevice, ze_device_handle_t hSubDevice,
    size_t _pDeviceProperties_val_length,
    ze_device_properties_t *pDeviceProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  hp_device_t hpd{hostname, vpid, (thapi_device_id)hSubDevice};
  data->device_property[hpd] = *pDeviceProperties_val;
  data->subdevice_parent[hpd] = (thapi_device_id)hDevice;
}

/*
 * Map command list to device
 */
static void command_list_entry(void *btx_handle, void *usr_data, int64_t ts,
                               const char *event_class_name,
                               const char *hostname, int64_t vpid,
                               uint64_t vtid,
                               const ze_device_handle_t hDevice) {
  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hDevice);
}

static void command_list_exit(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name,
                              const char *hostname, int64_t vpid, uint64_t vtid,
                              ze_command_list_handle_t phCommandList_val) {
  auto *data = static_cast<data_t *>(usr_data);
  const auto device =
      data->entry_state.get_data<thapi_device_id>({hostname, vpid, vtid});
  data->command_list_device[{hostname, vpid, phCommandList_val}] = device;
}

/*
 * Name of the Function Profiled
 */
static void zeKernelCreate_entry_callback(void *btx_handle, void *usr_data,
                                          int64_t ts,
                                          const char *event_class_name,
                                          const char *hostname, int64_t vpid,
                                          uint64_t vtid,
                                          char *desc__pKernelName_val) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid},
                             std::string{desc__pKernelName_val});
}

static void zeKernelCreate_exit_callback(void *btx_handle, void *usr_data,
                                         int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid,
                                         ze_kernel_handle_t phKernel_val) {

  auto *data = static_cast<data_t *>(usr_data);
  // No need to check for Error, if not success, people should will not use it.
  const auto kernelName =
      data->entry_state.get_data<std::string>({hostname, vpid, vtid});
  data->kernel_name[{hostname, vpid, phKernel_val}] = kernelName;
}

static void zeKernelSetGroupSize_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_kernel_handle_t hKernel,
    uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) {

  auto *data = static_cast<data_t *>(usr_data);
  std::stringstream groupsize;
  groupsize << "{" << groupSizeX << "," << groupSizeY << "," << groupSizeZ
            << "}";
  data->kernel_groupsize_str[{hostname, vpid, hKernel}] = groupsize.str();
}

static void
property_kernel_callback(void *btx_handle, void *usr_data, int64_t ts,
                         const char *hostname, int64_t vpid, uint64_t vtid,
                         ze_kernel_handle_t hKernel,
                         size_t _pKernelProperties_val_length,
                         ze_kernel_properties_t *pKernelProperties_val) {

  auto *data = static_cast<data_t *>(usr_data);
  // SIMD Size ~= maxGroupSize
  data->kernel_simdsize_str[{hostname, vpid, hKernel}] =
      "SIMD" + std::to_string(pKernelProperties_val->maxSubgroupSize);
}

/*
 * Profiling Command (everything who signal an event on completion)
 * */
static void hSignalEvent_launchKernel_with_group_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid,
    ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel,
    ze_group_count_t *pLaunchFuncArgs_val) {

  auto *data = static_cast<data_t *>(usr_data);

  const hp_kernel_t hpk{hostname, vpid, hKernel};
  std::string name = data->kernel_name[hpk];

  std::stringstream metadata;
  metadata << data->kernel_simdsize_str[hpk] << ", ";
  metadata << "{" << pLaunchFuncArgs_val->groupCountX << ","
           << pLaunchFuncArgs_val->groupCountY << ","
           << pLaunchFuncArgs_val->groupCountZ << "}"
           << ", " << data->kernel_groupsize_str[hpk];

  const auto device = data->command_list_device[{hostname, vpid, hCommandList}];
  // Not sure why need to store the command list and the device, as we never
  // remove anything from `command_list_device`
  data->command_partial_payload[{hostname, vpid, vtid}] = {
      hCommandList, name, metadata.str(), device, ts};
}

static void hSignalEvent_launchKernel_without_group_entry_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name,
    const char *hostname, int64_t vpid, uint64_t vtid,
    ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel) {

  auto *data = static_cast<data_t *>(usr_data);
  std::string name = data->kernel_name[{hostname, vpid, hKernel}];
  std::string metadata = "";

  const auto device = data->command_list_device[{hostname, vpid, hCommandList}];
  data->command_partial_payload[{hostname, vpid, vtid}] = {
      hCommandList, name, metadata, device, ts};
}

static void eventMemory2ptr_callback(void *btx_handle, void *usr_data,
                                     int64_t ts, const char *event_class_name,
                                     const char *hostname, int64_t vpid,
                                     uint64_t vtid,
                                     ze_command_list_handle_t hCommandList,
                                     void *dstptr, void *srcptr, size_t size) {

  auto *data = static_cast<data_t *>(usr_data);
  const hp_t hp{hostname, vpid};
  std::stringstream name;
  name << strip_event_class_name_entry(event_class_name) << "("
       << memory_location(data, hp, (uintptr_t)srcptr) << "2"
       << memory_location(data, hp, (uintptr_t)dstptr) << ")";
  std::string metadata = "";

  const auto device = data->command_list_device[{hostname, vpid, hCommandList}];
  data->command_partial_payload[{hostname, vpid, vtid}] = {
      hCommandList, name.str(), metadata, device, ts};

  // Should we check Error ? Send with Error?
  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, ts,
                                 BACKEND_ZE, name.str().c_str(), size);
}

static void eventMemory1ptr_callback(void *btx_handle, void *usr_data,
                                     int64_t ts, const char *event_class_name,
                                     const char *hostname, int64_t vpid,
                                     uint64_t vtid,
                                     ze_command_list_handle_t hCommandList,
                                     void *ptr, size_t size) {

  auto *data = static_cast<data_t *>(usr_data);
  const hp_t hp{hostname, vpid};
  std::stringstream name;
  name << strip_event_class_name_entry(event_class_name) << "("
       << memory_location(data, hp, (uintptr_t)ptr) << ")";

  std::string metadata = "";

  const auto device = data->command_list_device[{hostname, vpid, hCommandList}];
  data->command_partial_payload[{hostname, vpid, vtid}] = {
      hCommandList, name.str(), metadata, device, ts};

  // Should we check Error ? Send with Error?
  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, ts,
                                 BACKEND_ZE, name.str().c_str(), size);
}

static void memory_but_no_event_callback(void *btx_handle, void *usr_data,
                                         int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname, int64_t vpid,
                                         uint64_t vtid, size_t size) {

  btx_push_message_lttng_traffic(
      btx_handle, hostname, vpid, vtid, ts, BACKEND_ZE,
      strip_event_class_name_entry(event_class_name).c_str(), size);
}

// Barrier
static void hSignalEvent_rest_callback(void *btx_handle, void *usr_data,
                                       int64_t ts, const char *event_class_name,
                                       const char *hostname, int64_t vpid,
                                       uint64_t vtid,
                                       ze_command_list_handle_t hCommandList) {

  auto *data = static_cast<data_t *>(usr_data);
  std::string name = strip_event_class_name_entry(event_class_name);
  std::string metadata = "";

  const auto device = data->command_list_device[{hostname, vpid, hCommandList}];

  data->command_partial_payload[{hostname, vpid, vtid}] = {
      hCommandList, name, metadata, device, ts};
}

// Handle Global

// zeModuleGetGlobalPointer and zeModuleDestroy
static void zeModule_entry_callback(void *btx_handle, void *usr_data,
                                    int64_t ts, const char *event_class_name,
                                    const char *hostname, int64_t vpid,
                                    uint64_t vtid, ze_module_handle_t hModule) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hModule);
}

static void zeModuleGetGlobalPointer_exit_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_result_t zeResult, size_t pSize_val,
    void *pptr_val) {

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  auto *data = static_cast<data_t *>(usr_data);
  auto hModule =
      data->entry_state.get_data<ze_module_handle_t>({hostname, vpid, vtid});
  // Used to free memory
  data->module_global_pointer[{hostname, vpid, hModule}].insert(
      (uintptr_t)pptr_val);
  // Lets consider it like shared memory
  add_memory(data, {hostname, vpid}, (uintptr_t)pptr_val, pSize_val,
             "lttng_ust_ze:zeMemAllocShared_exit");
}

static void zeModuleDestroy_exit_callback(void *btx_handle, void *usr_data,
                                          int64_t ts, const char *hostname,
                                          int64_t vpid, uint64_t vtid,
                                          ze_result_t zeResult) {
  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  auto *data = static_cast<data_t *>(usr_data);
  auto hModule =
      data->entry_state.get_data<ze_module_handle_t>({hostname, vpid, vtid});

  auto &s = data->module_global_pointer;
  auto it = s.find({hostname, vpid, hModule});
  // This should never happen
  if (it == s.end())
    return;

  for (auto &s2 : it->second)
    data->rangeset_memory_shared[{hostname, vpid}].erase(s2);
  s.erase(it);
}

/*
 * Remove Memory
 */
static void memFree_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname, int64_t vpid,
                                   uint64_t vtid, void *ptr) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, ptr);
}

static void memFree_exit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                  const char *event_class_name,
                                  const char *hostname, int64_t vpid,
                                  uint64_t vtid, ze_result_t ze_result) {

  auto *data = static_cast<data_t *>(usr_data);
  if (ze_result == ZE_RESULT_SUCCESS) {
    auto ptr = data->entry_state.get_data<uintptr_t>({hostname, vpid, vtid});
    remove_memory(data, {hostname, vpid}, ptr);
  }
}

/*
 * Timestamp Shift
 */

static void property_device_timer_callback(void *btx_handle, void *usr_data,
                                           int64_t ts, const char *hostname,
                                           int64_t vpid, uint64_t vtid,
                                           ze_device_handle_t hDevice,
                                           uint64_t hostTimestamp,
                                           uint64_t deviceTimestamp) {
  auto *data = static_cast<data_t *>(usr_data);
  data->device_timestamps_pair_ref[{hostname, vpid, (thapi_device_id)hDevice}] =
      {ts, deviceTimestamp};
}

/*
 * Profiling Event
 */
static void event_profiling_callback(void *btx_handle, void *usr_data,
                                     int64_t ts, const char *hostname,
                                     int64_t vpid, uint64_t vtid,
                                     ze_event_handle_t hEvent) {

  auto *data = static_cast<data_t *>(usr_data);

  auto it_pp = data->command_partial_payload.find({hostname, vpid, vtid});
  // We didn't find the command who initiated this even_profiling,
  // This mean we should ignore it, Either mean nothing, instaneous
  // or not supported
  //
  // zeCommandListAppendSignalEvent|zeCommandListAppendLaunchMultipleKernelsIndirect...
  //
  if (it_pp == data->command_partial_payload.end())
    return;

  // Don't put an `&` is break the metadata / commandName... Not sure why
  const auto [hCommandList, commandName, metadata, device, lltngMin] =
      it_pp->second;
  data->command_partial_payload.erase(it_pp);

  // Got the timestamp pair reference
  clock_lttng_device_t clockLttngDevice;
  const auto &m0 = data->device_timestamps_pair_ref;
  const auto it0 = m0.find({hostname, vpid, device});
  if (it0 != m0.cend())
    clockLttngDevice = it0->second;
  // Do we handle the case where it's nul??
  data->event_payload[{hostname, vpid, hEvent}] = {
      vtid, commandName, metadata, device, lltngMin, clockLttngDevice};
}

static void event_profiling_result_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_event_handle_t hEvent, ze_result_t status,
    ze_result_t timestampStatus, uint64_t globalStart, uint64_t globalEnd,
    uint64_t contextStart, uint64_t contextEnd) {

  if (status == ZE_RESULT_NOT_READY)
    return;

  auto *data = static_cast<data_t *>(usr_data);

  // We didn't find the partial payload, that mean we should ignore it
  const auto it_p = data->event_payload.find({hostname, vpid, hEvent});
  if (it_p == data->event_payload.cend())
    return;

  const auto &[vtid_submission, commandName, metadata, device, lltngMin,
               clockLttngDevice] = it_p->second;
  // We don't erase, may have one entry for multiple result

  const bool err =
      ((status != ZE_RESULT_SUCCESS) || (timestampStatus != ZE_RESULT_SUCCESS));

  // No device information. No conversion to ns, no looping
  uint64_t delta = globalEnd - globalStart;
  uint64_t start = lltngMin;
  uintptr_t device_hash = 0;
  const auto it0 = data->device_property.find({hostname, vpid, device});
  if (it0 != data->device_property.cend()) {
    if (!err) {
      delta = compute_and_convert_delta(globalStart, globalEnd, it0->second);
      start = convert_device_cycle(globalStart, clockLttngDevice, lltngMin,
                                   it0->second);
    }
    device_hash = hash_device(it0->second);
  }
  uintptr_t subdevice_hash = 0;
  const auto it1 = data->subdevice_parent.find({hostname, vpid, device});
  if (it1 != data->subdevice_parent.cend()) {
    subdevice_hash = device_hash;
    const auto it2 = data->device_property.find({hostname, vpid, it1->second});
    if (it2 != data->device_property.cend())
      subdevice_hash = hash_device(it2->second);
  }

  btx_push_message_lttng_device(btx_handle, hostname, vpid, vtid_submission,
                                start, BACKEND_ZE, commandName.c_str(), delta,
                                device_hash, subdevice_hash, err,
                                metadata.c_str());
}

static void zeEventDestroy_entry_callback(void *btx_handle, void *usr_data,
                                          int64_t ts, const char *hostname,
                                          int64_t vpid, uint64_t vtid,
                                          ze_event_handle_t hEvent) {

  auto *data = static_cast<data_t *>(usr_data);
  data->entry_state.set_data({hostname, vpid, vtid}, hEvent);
}

static void zeEventDestroy_exit_callback(void *btx_handle, void *usr_data,
                                         int64_t ts, const char *hostname,
                                         int64_t vpid, uint64_t vtid,
                                         ze_result_t zeResult) {

  if (zeResult != ZE_RESULT_SUCCESS)
    return;

  auto *data = static_cast<data_t *>(usr_data);
  auto hEvent =
      data->entry_state.get_data<ze_event_handle_t>({hostname, vpid, vtid});
  data->event_payload.erase({hostname, vpid, hEvent});
}

/*
 * Sampling
 */

static void lttng_ust_ze_sampling_gpu_energy_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_device_handle_t hDevice, uint32_t domain,
    uint64_t energy, uint64_t sampling_ts) {

  btx_push_message_lttng_power(btx_handle, hostname, 0, 0, ts, BACKEND_ZE,
                               (uint64_t)hDevice, domain, energy);
}

static void lttng_ust_ze_sampling_gpu_frequency_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_device_handle_t hDevice, uint32_t domain,
    uint64_t frequency) {

  btx_push_message_lttng_frequency(btx_handle, hostname, 0, 0, ts, BACKEND_ZE,
                                   (uint64_t)hDevice, domain, frequency);
}

static void lttng_ust_ze_sampling_computeEngine_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_device_handle_t hDevice, uint32_t subDevice,
    uint64_t activeTime, uint64_t sampling_ts) {

  btx_push_message_lttng_computeEU(btx_handle, hostname, 0, 0, ts, BACKEND_ZE,
                                   (uint64_t)hDevice, subDevice, activeTime);
}

static void lttng_ust_ze_sampling_copyEngine_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ze_device_handle_t hDevice, uint32_t subDevice,
    uint64_t activeTime, uint64_t sampling_ts) {

  btx_push_message_lttng_copyEU(btx_handle, hostname, 0, 0, ts, BACKEND_ZE,
                                (uint64_t)hDevice, subDevice, activeTime);
}

/*
 * Register
 */
#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                \
  btx_register_callbacks_##base_name(btx_handle, &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle,
                                              &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle,
                                            &btx_finalize_component);

  /* Host*/
  REGISTER_ASSOCIATED_CALLBACK(entries);
  REGISTER_ASSOCIATED_CALLBACK(exits);

  /* Memory */
  btx_register_callbacks_entries_alloc(btx_handle, &entries_alloc_callback);
  btx_register_callbacks_exits_alloc(btx_handle, &exits_alloc_callback);
  btx_register_callbacks_zeModule_entry(btx_handle, &zeModule_entry_callback);

  /* Device and Subdevice property */
  btx_register_callbacks_lttng_ust_ze_properties_device(
      btx_handle, &property_device_callback);
  btx_register_callbacks_lttng_ust_ze_properties_subdevice(
      btx_handle, &property_subdevice_callback);

  /* Map command list to device */
  btx_register_callbacks_command_list_entry(btx_handle, &command_list_entry);
  btx_register_callbacks_command_list_exit(btx_handle, &command_list_exit);

  /*  Name of the Function Profiled  */
  btx_register_callbacks_zeKernelCreate_entry(btx_handle,
                                              &zeKernelCreate_entry_callback);
  btx_register_callbacks_zeKernelCreate_exit(btx_handle,
                                             &zeKernelCreate_exit_callback);

  btx_register_callbacks_lttng_ust_ze_zeKernelSetGroupSize_entry(
      btx_handle, &zeKernelSetGroupSize_entry_callback);
  btx_register_callbacks_lttng_ust_ze_properties_kernel(
      btx_handle, &property_kernel_callback);

  /* Drift */
  btx_register_callbacks_lttng_ust_ze_properties_device_timer(
      btx_handle, &property_device_timer_callback);

  /* Profiling Command (everything who signal an event on completion)  */
  btx_register_callbacks_hSignalEvent_hKernel_with_group_entry(
      btx_handle, &hSignalEvent_launchKernel_with_group_entry_callback);
  btx_register_callbacks_hSignalEvent_hKernel_without_group_entry(
      btx_handle, &hSignalEvent_launchKernel_without_group_entry_callback);
  btx_register_callbacks_hSignalEvent_eventMemory_2ptr_entry(
      btx_handle, &eventMemory2ptr_callback);
  btx_register_callbacks_hSignalEvent_eventMemory_1ptr_entry(
      btx_handle, &eventMemory1ptr_callback);
  btx_register_callbacks_eventMemory_without_hSignalEvent_entry(
      btx_handle, &memory_but_no_event_callback);
  btx_register_callbacks_hSignalEvent_rest_entry(btx_handle,
                                                 &hSignalEvent_rest_callback);

  /* Remove Memory */
  btx_register_callbacks_memFree_entry(btx_handle, &memFree_entry_callback);
  btx_register_callbacks_memFree_exit(btx_handle, &memFree_exit_callback);
  btx_register_callbacks_lttng_ust_ze_zeModuleGetGlobalPointer_exit(
      btx_handle, &zeModuleGetGlobalPointer_exit_callback);
  btx_register_callbacks_lttng_ust_ze_zeModuleDestroy_exit(
      btx_handle, &zeModuleDestroy_exit_callback);

  /* Handling of event */
  btx_register_callbacks_lttng_ust_ze_profiling_event_profiling(
      btx_handle, &event_profiling_callback);
  btx_register_callbacks_lttng_ust_ze_profiling_event_profiling_results(
      btx_handle, &event_profiling_result_callback);

  btx_register_callbacks_lttng_ust_ze_zeEventDestroy_entry(
      btx_handle, &zeEventDestroy_entry_callback);
  btx_register_callbacks_lttng_ust_ze_zeEventDestroy_exit(
      btx_handle, &zeEventDestroy_exit_callback);

  /* Sampling */
  btx_register_callbacks_lttng_ust_ze_sampling_gpu_energy(
      btx_handle, &lttng_ust_ze_sampling_gpu_energy_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_gpu_frequency(
      btx_handle, &lttng_ust_ze_sampling_gpu_frequency_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_computeEngine(
      btx_handle, &lttng_ust_ze_sampling_computeEngine_callback);
  btx_register_callbacks_lttng_ust_ze_sampling_copyEngine(
      btx_handle, &lttng_ust_ze_sampling_copyEngine_callback);
}
