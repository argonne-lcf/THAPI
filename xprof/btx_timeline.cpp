#include <metababel/metababel.h>

#include "xprof_utils.hpp" // typedef
#include <fstream>
#include <iomanip>  // set precision
#include <iostream> // stdcout
#include <map>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility> // pair

#include "perfetto_prunned.pb.h"

#define TRUSTED_PACKED_SEQUENCE_ID 10

using timestamp_t = uint64_t;
using perfetto_uuid_t = uint64_t;

// Based on https://perfetto.dev/docs/reference/synthetic-track-event

struct timeline_dispatch_s {
  // User params provided to the user component.
  btx_params_t *params;

  std::unordered_map<hp_dsd_t, perfetto_uuid_t> hp2uuid;
  std::unordered_map<std::pair<perfetto_uuid_t, thread_id_t>, perfetto_uuid_t> thread2uuid;
  std::unordered_map<perfetto_uuid_t, std::stack<timestamp_t>> uuid2stack;
  std::unordered_map<std::pair<perfetto_uuid_t, thread_id_t>,
                     std::map<timestamp_t, perfetto_uuid_t>>
      track2lasts;

  std::unordered_map<h_device_t, perfetto_uuid_t> hp_device2countertracks;
  std::unordered_map<h_ddomain_t, perfetto_uuid_t> hp_ddomain2telmtracks;
  std::unordered_map<h_ddomain_t, perfetto_uuid_t> hp_ddomain2cpyalloctracks;
  std::unordered_map<h_dfsdev_t, perfetto_uuid_t> hp_dfsdev2fptracks;
  perfetto_pruned::Trace trace;
};
// Keeps extra parameters that does not fit the default getter
using Extras = std::tuple<bool, uint32_t, uint32_t>;

using timeline_dispatch_t = struct timeline_dispatch_s;
using uuid_getter_t = perfetto_uuid_t (*)(timeline_dispatch_t *, const std::string &, uint64_t,
                                          uint32_t, uint64_t, uint32_t, std::optional<Extras>);

static perfetto_uuid_t gen_perfetto_uuid() {
  // Start at one, Look like UUID 0 is special
  static std::atomic<perfetto_uuid_t> uuid{1};
  return uuid++;
}

static perfetto_uuid_t get_parent_counter_track_uuid(timeline_dispatch_t *dispatch,
                                                     std::string hostname, thapi_device_id did,
                                                     uint32_t deviceIdx) {
  perfetto_uuid_t hp_uuid = 0;
  auto [it, inserted] = dispatch->hp_device2countertracks.insert({{hostname, did}, hp_uuid});
  auto &potential_uuid = it->second;
  // Exists
  if (!inserted)
    return potential_uuid;

  hp_uuid = gen_perfetto_uuid();
  potential_uuid = hp_uuid;

  // Create packet with track descriptor
  auto *packet = dispatch->trace.add_packet();
  packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
  packet->set_timestamp(0);
  // TODO: check if this is required
  packet->set_previous_packet_dropped(true);
  auto *track_descriptor = packet->mutable_track_descriptor();
  track_descriptor->set_uuid(hp_uuid);
  auto *process = track_descriptor->mutable_process();
  process->set_pid(hp_uuid);
  std::ostringstream oss;
  oss << "Hostname " << hostname << " | Device " << deviceIdx;
  // oss << " | " << track_name << " | uuid ";
  process->set_process_name(oss.str());
  return hp_uuid;
}

template <typename MapType, typename KeyType>
std::pair<perfetto_uuid_t, perfetto_uuid_t>
insert_or_get_uuid(MapType &counter_map, KeyType key, timeline_dispatch_t *dispatch,
                   const std::string &hostname, thapi_device_id did, uint32_t deviceIdx) {

  perfetto_uuid_t hp_dev_uuid = 0;
  auto [it, inserted] = counter_map.insert({key, hp_dev_uuid});
  auto &potential_uuid = it->second;

  if (!inserted)
    return {potential_uuid, 0}; // If present, return only hp_dev_uuid.

  // Generate both parent and device uuid if a new insertion
  perfetto_uuid_t hp_uuid = get_parent_counter_track_uuid(dispatch, hostname, did, deviceIdx);
  hp_dev_uuid = gen_perfetto_uuid();
  potential_uuid = hp_dev_uuid;

  return {hp_dev_uuid, hp_uuid};
}

static perfetto_uuid_t get_counter_track_uuuid(
    timeline_dispatch_t *dispatch,
    std::unordered_map<h_ddomain_t, perfetto_uuid_t> &counter_tracks,
    const std::string &track_name, const std::string &hostname, thapi_device_id did,
    uint32_t deviceIdx, uint64_t tHandle, thapi_domain_idx domain,
    std::optional<Extras> fabricExtras = std::nullopt,
    std::unordered_map<h_dfsdev_t, perfetto_uuid_t> *counter_tracks_fp = nullptr) {

  // Choose key and map
  std::pair<perfetto_uuid_t, perfetto_uuid_t> uuids;
  if (fabricExtras && counter_tracks_fp) {
    bool RxTx = std::get<0>(*fabricExtras);
    auto key = std::make_tuple(hostname, did, tHandle, domain, RxTx);
    uuids = insert_or_get_uuid(*counter_tracks_fp, key, dispatch, hostname, did, deviceIdx);
  } else {
    auto key = std::make_tuple(hostname, did, domain, tHandle);
    uuids = insert_or_get_uuid(counter_tracks, key, dispatch, hostname, did, deviceIdx);
  }

  // Get hp_dev_uuid and hp_uuid
  perfetto_uuid_t hp_dev_uuid = uuids.first;
  perfetto_uuid_t hp_uuid = uuids.second;

  // Packet creation (independent of the map used for UUID storage)
  auto *packet = dispatch->trace.add_packet();
  packet->set_timestamp(0);
  packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
  auto *track_descriptor = packet->mutable_track_descriptor();
  track_descriptor->set_uuid(hp_dev_uuid);

  // Set the parent UUID if it was generated
  if (hp_uuid != 0) {
    track_descriptor->set_parent_uuid(hp_uuid);
  }

  std::ostringstream oss;
  if (track_name == "FabricT" && fabricExtras) {
    bool RxTx = std::get<0>(*fabricExtras);
    uint32_t fabricId = std::get<1>(*fabricExtras);
    uint32_t remotePortId = std::get<2>(*fabricExtras);
    oss << track_name << " | SD " << domain << " | " << fabricId << "<->" << remotePortId << " | "
        << (RxTx ? " TX" : " RX");
  } else if (track_name == "Memory") {
    oss << track_name << " BW | Module " << domain;
  } else if (track_name == "Allocated Memory (%)") {
    oss << track_name << " Module " << domain;
  } else if (track_name == "CopyEngine (%)" || track_name == "ComputeEngine (%)") {
    oss << track_name << " | SubDevice " << domain;
  } else if (track_name == "Power") {
    if (domain == 0) {
      oss << "    Total Power";
    } else {
      oss << track_name << " | SubDevice " << domain - 1;
    }
  } else {
    oss << track_name << " | SubDevice " << domain;
  }

  track_descriptor->set_name(oss.str());
  track_descriptor->mutable_counter();

  return hp_dev_uuid;
}

static perfetto_uuid_t get_copyEU_track_uuuid(timeline_dispatch_t *dispatch,
                                              const std::string &hostname, uint64_t did,
                                              uint32_t deviceIdx, uint64_t hEngine,
                                              uint32_t subDevice, std::optional<Extras> options) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2cpyalloctracks, "CopyEngine (%)",
                                 hostname, did, deviceIdx, hEngine, subDevice);
}

static perfetto_uuid_t get_computeEU_track_uuuid(timeline_dispatch_t *dispatch,
                                                 const std::string &hostname, uint64_t did,
                                                 uint32_t deviceIdx, uint64_t hEngine,
                                                 uint32_t subDevice,
                                                 std::optional<Extras> options) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2telmtracks, "ComputeEngine (%)",
                                 hostname, did, deviceIdx, hEngine, subDevice);
}

static perfetto_uuid_t get_fpThroughput_track_uuuid(timeline_dispatch_t *dispatch,
                                                    const std::string &hostname, uint64_t did,
                                                    uint32_t deviceIdx, uint64_t hFabricPort,
                                                    uint32_t subDevice,
                                                    std::optional<Extras> fabricExtras) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2telmtracks, "FabricT", hostname,
                                 did, deviceIdx, hFabricPort, subDevice, fabricExtras,
                                 &dispatch->hp_dfsdev2fptracks);
}

static perfetto_uuid_t get_power_track_uuuid(timeline_dispatch_t *dispatch,
                                             const std::string &hostname, uint64_t did,
                                             uint32_t deviceIdx, uint64_t hPower,
                                             uint32_t subDevice, std::optional<Extras> options) {
  // Extra space to maintain track sequence in the timeline
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2telmtracks, "Power", hostname, did,
                                 deviceIdx, hPower, subDevice);
}

static perfetto_uuid_t get_frequency_track_uuuid(timeline_dispatch_t *dispatch,
                                                 const std::string &hostname, uint64_t did,
                                                 uint32_t deviceIdx, uint64_t hFrequency,
                                                 uint32_t subDevice,
                                                 std::optional<Extras> options) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2telmtracks, "Frequency", hostname,
                                 did, deviceIdx, hFrequency, subDevice);
}

static perfetto_uuid_t get_bandwidth_track_uuuid(timeline_dispatch_t *dispatch,
                                                 const std::string &hostname, uint64_t did,
                                                 uint32_t deviceIdx, uint64_t hMemModule,
                                                 uint32_t subDevice,
                                                 std::optional<Extras> options) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2telmtracks, "Memory", hostname, did,
                                 deviceIdx, hMemModule, subDevice);
}

static perfetto_uuid_t get_allocation_track_uuuid(timeline_dispatch_t *dispatch,
                                                  const std::string &hostname, uint64_t did,
                                                  uint32_t deviceIdx, uint64_t hMemModule,
                                                  uint32_t subDevice,
                                                  std::optional<Extras> options) {
  return get_counter_track_uuuid(dispatch, dispatch->hp_ddomain2cpyalloctracks,
                                 "Allocated Memory (%)", hostname, did, deviceIdx, hMemModule,
                                 subDevice);
}

static void add_event_DTelemetry(timeline_dispatch_t *dispatch, const std::string &hostname,
                                 uint64_t did, uint32_t deviceIdx, uint64_t tHandle,
                                 uint32_t subDevice, uint64_t timestamp, float value,
                                 uuid_getter_t uuid_getter, const std::string &eventName,
                                 std::optional<Extras> options = std::nullopt) {
  perfetto_uuid_t track_uuid;
  track_uuid = uuid_getter(dispatch, hostname, did, deviceIdx, tHandle, subDevice, options);

  auto *packet = dispatch->trace.add_packet();
  packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
  packet->set_timestamp(timestamp);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_COUNTER);
  track_event->set_track_uuid(track_uuid);
  track_event->set_double_counter_value(value);
}

static void add_event_memModule(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                                uint32_t deviceIdx, uintptr_t hMemModule, uint32_t subDevice,
                                uint64_t timestamp, float pBandwidth, float rdBandwidth,
                                float wtBandwidth, float allocation) {
  // Define for RX throughput.
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hMemModule, subDevice, timestamp,
                       pBandwidth, get_bandwidth_track_uuuid, "Memory BW");

  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hMemModule, subDevice, timestamp,
                       allocation, get_allocation_track_uuuid, "Memory Allocation");
}

static void add_event_fabricPort(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                                 uint32_t deviceIdx, uintptr_t hFabricPort, uint32_t subDevice,
                                 uint64_t timestamp, uint32_t fabricId, uint32_t remotePortId,
                                 float rxThroughput, float txThroughput, float rxSpeed,
                                 float txSpeed) {
  // Define.
  Extras fabricExtras = {false, fabricId, remotePortId};
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hFabricPort, subDevice, timestamp,
                       rxThroughput, get_fpThroughput_track_uuuid, "Fabric ThroughputRX",
                       fabricExtras);

  fabricExtras = {true, fabricId, remotePortId};
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hFabricPort, subDevice, timestamp,
                       txThroughput, get_fpThroughput_track_uuuid, "Fabric ThroughputTX",
                       fabricExtras);
}

static void add_event_frequency(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                                uint32_t deviceIdx, uint64_t hFrequency, uint32_t subDevice,
                                uint64_t timestamp, float frequency) {
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hFrequency, subDevice, timestamp,
                       frequency, get_frequency_track_uuuid, "Frequency");
}

static void add_event_power(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                            uint32_t deviceIdx, uint64_t hPower, uint32_t subDevice,
                            uint64_t timestamp, float power) {
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hPower, subDevice, timestamp, power,
                       get_power_track_uuuid, "Power");
}

static void add_event_computeEU(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                                uint32_t deviceIdx, uint64_t hEngine, uint32_t subDevice,
                                uint64_t timestamp, float activeTime) {
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hEngine, subDevice, timestamp,
                       activeTime, get_computeEU_track_uuuid, "ComputeEngine");
}

static void add_event_copyEU(timeline_dispatch_t *dispatch, std::string hostname, uint64_t did,
                             uint32_t deviceIdx, uint64_t hEngine, uint32_t subDevice,
                             uint64_t timestamp, float activeTime) {
  add_event_DTelemetry(dispatch, hostname, did, deviceIdx, hEngine, subDevice, timestamp,
                       activeTime, get_copyEU_track_uuuid, "CopyEngine");
}

static void add_event_begin(timeline_dispatch_t *dispatch, perfetto_uuid_t uuid, timestamp_t begin,
                            std::string name) {
  auto *packet = dispatch->trace.add_packet();
  packet->set_timestamp(begin);
  packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_BEGIN);
  track_event->set_name(name);
  track_event->set_track_uuid(uuid);
}

static void add_event_end(timeline_dispatch_t *dispatch, perfetto_uuid_t uuid, uint64_t end) {
  auto *packet = dispatch->trace.add_packet();
  packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
  packet->set_timestamp(end);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_END);
  track_event->set_track_uuid(uuid);
}

static perfetto_uuid_t get_process_uuid(timeline_dispatch_t *dispatch, std::string hostname,
                                        uint64_t process_id,
                                        std::optional<thapi_device_id> did = std::nullopt,
                                        std::optional<thapi_device_id> sdid = std::nullopt) {

  // Check if this uuid is already used
  perfetto_uuid_t hp_uuid = 0;
  auto r = dispatch->hp2uuid.insert(
      {{hostname, process_id, did.value_or(UINTPTR_MAX), sdid.value_or(UINTPTR_MAX)}, hp_uuid});
  auto &potential_uuid = r.first->second;
  // Process UUID already in the MAP
  if (!r.second)
    return potential_uuid;

  // Generating a new one
  hp_uuid = gen_perfetto_uuid();
  // Adding it to the map
  potential_uuid = hp_uuid;

  // Add the process packet to the trace
  {
    auto *packet = dispatch->trace.add_packet();

    auto *track_descriptor = packet->mutable_track_descriptor();
    track_descriptor->set_uuid(hp_uuid);
    auto *process = track_descriptor->mutable_process();

    // Use the same `pid` as uuid, because of hostname.
    process->set_pid(hp_uuid);
    std::ostringstream oss;
    oss << "Hostname " << hostname << " | Process " << process_id;
    if (did) {
      oss << " | Device " << *did;
      if (sdid)
        oss << " | SubDevice " << *sdid;
    }
    oss << " | uuid ";
    process->set_process_name(oss.str());
  }
  return hp_uuid;
}

// Perfectly Nested

static perfetto_uuid_t get_track_uuid_perfecly_nested(timeline_dispatch_t *dispatch,
                                                      std::string hostname, uint64_t process_id,
                                                      uint64_t thread_id) {

  // Get process UUID
  auto hp_uuid = get_process_uuid(dispatch, hostname, process_id);

  // Check if this uuid is already used
  //    variable initialized to avoid false positive in `-Werror=maybe-uninitialized`
  perfetto_uuid_t track_uuid = 0;
  auto r = dispatch->thread2uuid.insert({{hp_uuid, thread_id}, track_uuid});
  auto &potential_uuid = r.first->second;
  // Process UUID already in the mao
  if (!r.second)
    return potential_uuid;

  // Generating a new one
  track_uuid = gen_perfetto_uuid();
  // Adding it to the map
  potential_uuid = track_uuid;
  // Add the thread packet to the trace
  {
    auto *packet = dispatch->trace.add_packet();
    auto *track_descriptor = packet->mutable_track_descriptor();
    track_descriptor->set_uuid(track_uuid);
    track_descriptor->set_parent_uuid(hp_uuid);
    auto *thread = track_descriptor->mutable_thread();
    // Our `pid` is the same as the hp_uuid, because of hostname.
    thread->set_pid(hp_uuid);
    thread->set_tid(thread_id);
    // Add Thread name
    std::ostringstream oss;
    oss << "Thread ";
    thread->set_thread_name(oss.str());
  }
  return track_uuid;
}

static void add_event_perfectly_nested(timeline_dispatch_t *dispatch, std::string hostname,
                                       uint64_t process_id, uint64_t thread_id, std::string name,
                                       uint64_t begin, uint64_t dur) {

  auto track_uuid = get_track_uuid_perfecly_nested(dispatch, hostname, process_id, thread_id);

  const uint64_t end = begin + dur;
  // Handling perfectly nested event
  add_event_begin(dispatch, track_uuid, begin, name);
  std::stack<uint64_t> &s = dispatch->uuid2stack[track_uuid];
  while ((!s.empty()) && (s.top() <= begin)) {
    add_event_end(dispatch, track_uuid, s.top());
    s.pop();
  }
  s.push(end);
}

// Async
static perfetto_uuid_t get_track_uuid_async(timeline_dispatch_t *dispatch, std::string hostname,
                                            uint64_t process_id, thapi_device_id did,
                                            thapi_device_id sdid, uint64_t thread_id,
                                            uint64_t begin, uint64_t end) {

  auto process_uuid = get_process_uuid(dispatch, hostname, process_id, did, sdid);
  auto &lasts = dispatch->track2lasts[{process_uuid, thread_id}];

  perfetto_uuid_t uuid;
  auto it = lasts.upper_bound(begin);
  // Pre-historical event
  if (it == lasts.begin()) {
    uuid = gen_perfetto_uuid();
    {
      auto *packet = dispatch->trace.add_packet();
      auto *track_descriptor = packet->mutable_track_descriptor();
      track_descriptor->set_uuid(uuid);
      track_descriptor->set_parent_uuid(process_uuid);
      std::ostringstream oss;
      oss << "Thread " << thread_id;
      track_descriptor->set_name(oss.str());
    }
  } else {
    // Find the uuid who finished just before this one
    auto itp = std::prev(it);
    uuid = itp->second;
    lasts.erase(itp);
  }
  return lasts[end] = uuid;
}

static void add_event_async(timeline_dispatch_t *dispatch, std::string hostname,
                            uint64_t process_id, uint64_t thread_id, thapi_device_id did,
                            thapi_device_id sdid, std::string name, uint64_t begin, uint64_t dur) {

  auto end = begin + dur;
  auto track_uuid =
      get_track_uuid_async(dispatch, hostname, process_id, did, sdid, thread_id, begin, end);
  add_event_begin(dispatch, track_uuid, begin, name);
  add_event_end(dispatch, track_uuid, end);
}

void btx_initialize_component_callback(void **usr_data) { *usr_data = new timeline_dispatch_t; }

static void read_params_callback(void *usr_data, btx_params_t *usr_params) {
  auto *data = static_cast<timeline_dispatch_t *>(usr_data);
  data->params = usr_params;
}

void btx_finalize_component_callback(void *usr_data) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  for (auto &[uuid, s] : dispatch->uuid2stack) {
    while (!s.empty()) {
      add_event_end(dispatch, uuid, s.top());
      s.pop();
    }
  }

  std::string path{dispatch->params->output_path};
  if (path.empty()) {
    path = "out.pftrace";
  }

  // Write the new address book back to disk.
  std::fstream output(path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!dispatch->trace.SerializeToOstream(&output))
    std::cerr << "THAPI: Failed to write the trace at location: " << path << std::endl;
  else
    std::cout << "THAPI: Perfetto trace location: " << path << std::endl;
  google::protobuf::ShutdownProtobufLibrary();

  delete dispatch;
}

static void host_usr_callback(void *btx_handle, void *usr_data, const char *hostname, int64_t vpid,
                              uint64_t vtid, int64_t ts, int64_t backend_id, const char *name,
                              uint64_t dur, bt_bool err) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_perfectly_nested(dispatch, hostname, vpid, vtid, name, ts, dur);
}

static void device_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend,
                                const char *name, uint64_t dur, uint64_t did, uint64_t sdid,
                                bt_bool err, const char *metadata) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_async(dispatch, hostname, vpid, vtid, did, sdid, name, ts, dur);
}

static void frequency_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                   int64_t ts, uint64_t did, uint32_t deviceIdx,
                                   uint64_t hFrequency, uint32_t domain, uint64_t frequency) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_frequency(dispatch, hostname, did, deviceIdx, hFrequency, domain, ts, frequency);
}

static void power_usr_callback(void *btx_handle, void *usr_data, const char *hostname, int64_t ts,
                               uint64_t did, uint32_t deviceIdx, uint64_t hPower, uint32_t domain,
                               uint64_t power) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_power(dispatch, hostname, did, hPower, deviceIdx, domain, ts, power);
}

static void computeEU_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                   int64_t ts, uint64_t did, uint32_t deviceIdx, uint64_t hEngine,
                                   uint32_t subDevice, float activeTime) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_computeEU(dispatch, hostname, did, deviceIdx, hEngine, subDevice, ts, activeTime);
}

static void copyEU_usr_callback(void *btx_handle, void *usr_data, const char *hostname, int64_t ts,
                                uint64_t did, uint32_t deviceIdx, uint64_t hEngine,
                                uint32_t subDevice, float activeTime) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_copyEU(dispatch, hostname, did, deviceIdx, hEngine, subDevice, ts, activeTime);
}

static void fabricPort_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                    int64_t ts, uint64_t did, uint32_t deviceIdx,
                                    uint64_t hFabricPort, uint32_t subDevice, uint32_t fabricId,
                                    uint32_t remotePortId, float rxThroughput, float txThroughput,
                                    float rxSpeed, float txSpeed) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_fabricPort(dispatch, hostname, did, deviceIdx, hFabricPort, subDevice, ts, fabricId,
                       remotePortId, rxThroughput, txThroughput, rxSpeed, txSpeed);
}

static void memModule_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                   int64_t ts, uint64_t did, uint32_t deviceIdx,
                                   uint64_t hMemModule, uint32_t subDevice, float pBandwidth,
                                   float rdBandwidth, float wtBandwidth, float allocation) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  add_event_memModule(dispatch, hostname, did, deviceIdx, hMemModule, subDevice, ts, pBandwidth,
                      rdBandwidth, wtBandwidth, allocation);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_lttng_host(btx_handle, &host_usr_callback);
  btx_register_callbacks_lttng_device(btx_handle, &device_usr_callback);
  btx_register_callbacks_sampling_frequency(btx_handle, &frequency_usr_callback);
  btx_register_callbacks_sampling_power(btx_handle, &power_usr_callback);
  btx_register_callbacks_sampling_computeEU(btx_handle, &computeEU_usr_callback);
  btx_register_callbacks_sampling_copyEU(btx_handle, &copyEU_usr_callback);
  btx_register_callbacks_sampling_fabricPort(btx_handle, &fabricPort_usr_callback);
  btx_register_callbacks_sampling_memModule(btx_handle, &memModule_usr_callback);
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component_callback);
  btx_register_callbacks_read_params(btx_handle, &read_params_callback);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component_callback);
}
