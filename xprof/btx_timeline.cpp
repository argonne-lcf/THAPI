#include <metababel/metababel.h>

#include "xprof_utils.hpp" // typedef
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include "perfetto_prunned.pb.h"

#define TRUSTED_PACKED_SEQUENCE_ID 88

// A Trach where every MAX_EVENT_PER_TRACE_CHUNK packets will be flushed to disk
// And who contain a little utility to Interns String
class UnboundTrace {

public:
  UnboundTrace(std::string &output_path) : output_path_(output_path) {
    // Overwrite previous output_path file if existing
    std::fstream output(output_path, std::ios::out | std::ios::trunc | std::ios::binary);
    std::cout << "THAPI: Perfetto trace location: " << output_path_ << std::endl;
  }

  ::perfetto_pruned::TracePacket *add_packet() {
    // /!\ Assume all previous packets are ready to be serialized
    if (current_packet_count_ >= MAX_EVENT_PER_TRACE_CHUNK)
      serialize();
    current_packet_count_++;
    return trace_.add_packet();
  }

  ~UnboundTrace() {
    if (current_packet_count_ > 0)
      serialize();
  }

  std::optional<uint64_t> get_interned_id(::perfetto_pruned::TracePacket *packet,
                                          const std::string &name) {

    if (name.size() <= 8)
      return std::nullopt;

    // Sequence Flags:
    // The packet calling `get_interned_id` will used interned string
    int flags = perfetto_pruned::TracePacket::SEQ_NEEDS_INCREMENTAL_STATE;
    // If it the first packet to use interned string we need to clean the state
    if (name_to_iid_.empty()) {
      flags = flags | perfetto_pruned::TracePacket::SEQ_INCREMENTAL_STATE_CLEARED;
    }
    packet->set_sequence_flags(flags);

    // Creation of the interned string, Start at 1
    auto [it, inserted] =
        name_to_iid_.try_emplace(name, static_cast<uint64_t>(name_to_iid_.size()) + 1);
    uint64_t iid = it->second;

    if (inserted) {
      auto *interned_data = packet->mutable_interned_data();
      auto *entry = interned_data->add_event_names();
      entry->set_iid(iid);
      entry->set_name(name);
    }
    return iid;
  }

  uint64_t track_count = 0;

private:
  static constexpr unsigned MAX_EVENT_PER_TRACE_CHUNK = 100'000;
  std::string output_path_;
  ::perfetto_pruned::Trace trace_{};
  std::unordered_map<std::string, uint64_t> name_to_iid_;
  unsigned current_packet_count_ = 0;

  void serialize() {
    // /!\ Data will be appended to the output_path file
    std::fstream output(output_path_, std::ios::out | std::ios::app | std::ios::binary);
    if (!trace_.SerializeToOstream(&output))
      std::cerr << "THAPI: Failed to write the trace at location: " << output_path_ << std::endl;
    // Clean state
    trace_.clear_packet();
    current_packet_count_ = 0;
  }
};

class Track {
  // A (root no uuid)
  // |-> B1 (begin event slices)
  // |-> B2
  // |-> B (Empty track constructed by get_child)
  //   |-> C (Counter Track)
public:
  // Root constructor
  static Track make_root(std::shared_ptr<UnboundTrace> trace, uint64_t uuid_offset) {
    return Track(RootTag{}, trace, uuid_offset);
  }

  // Should not be used externally
  // But required for `try_emplace`
  Track(std::string name,
        std::optional<uint64_t> parent_uuid,
        std::shared_ptr<UnboundTrace> trace_ptr,
        bool is_leaf_counter = false)
      : name_(name), trace_ptr_(trace_ptr), parent_uuid_(parent_uuid),
        is_leaf_counter_(is_leaf_counter), uuid_(trace_ptr_->track_count++) {

    auto *packet = trace_ptr_->add_packet();
    packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
    auto *td = packet->mutable_track_descriptor();
    td->set_name(name_);
    td->set_uuid(uuid_.value());
    if (parent_uuid_)
      td->set_parent_uuid(parent_uuid_.value());

    if (is_leaf_counter_)
      td->mutable_counter();
  }

  void add_event_slice(std::string name,
                       uint64_t begin,
                       uint64_t end,
                       const std::vector<std::string> &track_names = {}) {

    const auto uuid = get_leaf(track_names).get_slice_uuid(begin, end);
    {
      auto *packet = trace_ptr_->add_packet();
      packet->set_timestamp(begin);
      packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);

      auto *track_event = packet->mutable_track_event();
      track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_BEGIN);
      track_event->set_track_uuid(uuid);

      const auto iid = trace_ptr_->get_interned_id(packet, name);
      if (iid)
        track_event->set_name_iid(iid.value());
      else
        track_event->set_name(name);
    }

    {
      auto *packet = trace_ptr_->add_packet();
      packet->set_timestamp(end);
      packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);

      auto *track_event = packet->mutable_track_event();
      track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_END);
      track_event->set_track_uuid(uuid);
    }
  }

  void add_event_counter(uint64_t timestamp,
                         double value,
                         const std::vector<std::string> &track_names = {}) {

    const auto uuid = get_leaf(track_names, true).uuid_.value();
    {
      auto *packet = trace_ptr_->add_packet();
      packet->set_timestamp(timestamp);
      packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);

      auto *track_event = packet->mutable_track_event();
      track_event->set_type(perfetto_pruned::TrackEvent::TYPE_COUNTER);
      track_event->set_track_uuid(uuid);
      track_event->set_double_counter_value(value);
    }
  }

private:
  struct RootTag {}; // private tag to restrict root creation
  explicit Track(RootTag, std::shared_ptr<UnboundTrace> trace_ptr, uint64_t uuid_offset)
      : name_("root"), trace_ptr_(trace_ptr), is_leaf_counter_(false) {}

  std::string name_;
  std::shared_ptr<UnboundTrace> trace_ptr_;

  // First Level have No parent_uuid (as root have no uuid)
  std::optional<uint64_t> parent_uuid_;
  // Leaf Counter Track need to set some special parameter to protobuf
  bool is_leaf_counter_;

  // Root Have no UUID
  std::optional<uint64_t> uuid_;

  // Children are empty Track or counter Track
  std::unordered_map<std::string, Track> childrens_;
  // Begins Are For Slice (who assume, for simplicity are assumed to non perfectly nested)
  std::map<uint64_t, Track> begins_;

  uint64_t get_slice_uuid(uint64_t begin, uint64_t end) {
    if (is_leaf_counter_)
      throw std::invalid_argument("Cannot get slice uuid for counter track");

    auto it = begins_.upper_bound(begin);
    if (it == begins_.begin()) {
      // Pre-historical event: create new track descriptor.
      // They share our parent. Our trace will be an empty track, but ready for other children to
      // use.
      auto new_it = begins_.emplace_hint(it, end, Track(name_, parent_uuid_, trace_ptr_));
      return (new_it->second.uuid_).value();
    } else {
      // Move the `uuid` who started before this events to the end
      auto itp = std::prev(it);

      // The doc said extract is the only way to change a key of a map element
      // without reallocation
      // https://en.cppreference.com/w/cpp/container/map/extract.html
      auto node = begins_.extract(itp);
      node.key() = end;
      auto result = begins_.insert(std::move(node));
      return (result.position->second.uuid_).value();
    }
  }

  inline Track &get_child(const std::string &name, bool is_leaf_counter = false) {
    auto [it, inserted] = childrens_.try_emplace(name, name, uuid_, trace_ptr_, is_leaf_counter);
    if (!inserted && it->second.is_leaf_counter_ != is_leaf_counter) {
      throw std::invalid_argument("Asked for a type (counter or slice) got something else");
    }
    return it->second;
  }

  inline Track &get_leaf(const std::vector<std::string> &names, bool is_leaf_counter = false) {
    Track *t = this;
    if (names.empty())
      return *t;

    // Iterate over all except the last
    for (size_t i = 0; i < names.size() - 1; ++i)
      t = &t->get_child(names[i]);

    // Pass is_leaf_counter
    return t->get_child(names.back(), is_leaf_counter);
  }
};

struct timeline_dispatch_s {
  std::unique_ptr<Track> track_tree;
};

using timeline_dispatch_t = struct timeline_dispatch_s;

void btx_initialize_component_callback(void **usr_data) { *usr_data = new timeline_dispatch_t; }

static void read_params_callback(void *usr_data, btx_params_t *usr_params) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  std::string output_path{usr_params->output_path};
  auto trace = std::make_shared<UnboundTrace>(output_path);
  dispatch->track_tree = std::make_unique<Track>(Track::make_root(trace, usr_params->offset));
}

void btx_finalize_component_callback(void *usr_data) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  delete dispatch;
  google::protobuf::ShutdownProtobufLibrary();
}

static void host_usr_callback(void *btx_handle,
                              void *usr_data,
                              const char *hostname,
                              int64_t vpid,
                              uint64_t vtid,
                              int64_t ts,
                              int64_t backend_id,
                              const char *name,
                              uint64_t dur,
                              bt_bool err) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_slice(
      name, ts, ts + dur,
      {hostname, "Process " + std::to_string(vpid), "Thread " + std::to_string(vtid)});
}

static void device_usr_callback(void *btx_handle,
                                void *usr_data,
                                const char *hostname,
                                int64_t vpid,
                                uint64_t vtid,
                                int64_t ts,
                                int64_t backend,
                                const char *name,
                                uint64_t dur,
                                uint64_t did,
                                uint64_t sdid,
                                bt_bool err,
                                const char *metadata) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_slice(
      name, ts, ts + dur,
      {hostname, "Process " + std::to_string(vpid), "Thread " + std::to_string(vtid),
       "Device " + std::to_string(did), "SubDevice " + std::to_string(sdid)});
}

static void frequency_usr_callback(void *btx_handle,
                                   void *usr_data,
                                   const char *hostname,
                                   int64_t ts,
                                   uint64_t did,
                                   uint32_t /*deviceIdx*/,
                                   uint64_t /*hFrequency*/,
                                   uint32_t domain,
                                   uint64_t frequency) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_counter(ts, static_cast<double>(frequency),
                                          {hostname, "ZE Counter", "Device " + std::to_string(did),
                                           "SubDevice " + std::to_string(domain), "Frequency"});
}

static void power_usr_callback(void *btx_handle,
                               void *usr_data,
                               const char *hostname,
                               int64_t ts,
                               uint64_t did,
                               uint32_t /*deviceIdx*/,
                               uint64_t /*hPower*/,
                               uint32_t domain,
                               uint64_t power) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  if (domain == 0) {
    dispatch->track_tree->add_event_counter(
        ts, static_cast<double>(power),
        {hostname, "ZE Counter", "Device " + std::to_string(did), "Power"});
  } else {

    dispatch->track_tree->add_event_counter(ts, static_cast<double>(power),
                                            {hostname, "ZE Counter",
                                             "Device " + std::to_string(did),
                                             "SubDevice " + std::to_string(domain - 1), "Power"});
  }
}

static void computeEU_usr_callback(void *btx_handle,
                                   void *usr_data,
                                   const char *hostname,
                                   int64_t ts,
                                   uint64_t did,
                                   uint32_t /*deviceIdx*/,
                                   uint64_t /*hEngine*/,
                                   uint32_t subDevice,
                                   float activeTime) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_counter(ts, static_cast<double>(activeTime),
                                          {hostname, "ZE Counter", "Device " + std::to_string(did),
                                           "SubDevice " + std::to_string(subDevice),
                                           "ComputeEngine (%)"});
}

static void copyEU_usr_callback(void *btx_handle,
                                void *usr_data,
                                const char *hostname,
                                int64_t ts,
                                uint64_t did,
                                uint32_t /*deviceIdx*/,
                                uint64_t /*hEngine*/,
                                uint32_t subDevice,
                                float activeTime) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_counter(ts, static_cast<double>(activeTime),
                                          {hostname, "ZE Counter", "Device " + std::to_string(did),
                                           "SubDevice " + std::to_string(subDevice),
                                           "CopyEngine (%)"});
}

static void fabricPort_usr_callback(void *btx_handle,
                                    void *usr_data,
                                    const char *hostname,
                                    int64_t ts,
                                    uint64_t did,
                                    uint32_t deviceIdx,
                                    uint64_t hFabricPort,
                                    uint32_t subDevice,
                                    uint32_t fabricId,
                                    uint32_t remotePortId,
                                    float rxThroughput,
                                    float txThroughput,
                                    float /*rxSpeed*/,
                                    float /*txSpeed*/) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_counter(
      ts, static_cast<double>(rxThroughput),
      {hostname, "ZE Counter", "Device " + std::to_string(did),
       "SubDevice " + std::to_string(subDevice),
       std::to_string(fabricId) + " <->" + std::to_string(remotePortId), "RX"});

  dispatch->track_tree->add_event_counter(
      ts, static_cast<double>(txThroughput),
      {hostname, "ZE Counter", "Device " + std::to_string(did),
       "SubDevice " + std::to_string(subDevice),
       std::to_string(fabricId) + " <->" + std::to_string(remotePortId), "TX"});
}

static void memModule_usr_callback(void *btx_handle,
                                   void *usr_data,
                                   const char *hostname,
                                   int64_t ts,
                                   uint64_t did,
                                   uint32_t /*deviceIdx*/,
                                   uint64_t /*hMemModule*/,
                                   uint32_t subDevice,
                                   float pBandwidth,
                                   float /*rdBandwidth*/,
                                   float /*wtBandwidth*/,
                                   float allocation) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->track_tree->add_event_counter(ts, static_cast<double>(pBandwidth),
                                          {hostname, "ZE Counter", "Device " + std::to_string(did),
                                           "SubDevice " + std::to_string(subDevice), "Memory BW"});

  dispatch->track_tree->add_event_counter(ts, static_cast<double>(allocation),
                                          {hostname, "ZE Counter", "Device " + std::to_string(did),
                                           "SubDevice " + std::to_string(subDevice),
                                           "Allocated Memory (%)"});
}

static void nic_usr_callback(void *btx_handle,
                             void *usr_data,
                             const char *hostname,
                             int64_t ts,
                             const char *interface_name,
                             const char *counter,
                             uint64_t value) {

  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  dispatch->track_tree->add_event_counter(
      ts, static_cast<double>(value),
      {hostname, "NIC Counter", "Interface " + std::string{interface_name}, counter});
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
  btx_register_callbacks_sampling_nic(btx_handle, &nic_usr_callback);
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component_callback);
  btx_register_callbacks_read_params(btx_handle, &read_params_callback);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component_callback);
}
