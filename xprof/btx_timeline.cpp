#include <metababel/metababel.h>

#include "xprof_utils.hpp" // typedef
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include "perfetto_prunned.pb.h"

#define TRUSTED_PACKED_SEQUENCE_ID 88

class Track;
// A Trach where every MAX_EVENT_PER_TRACE_CHUNK packets will be flushed to disk
// And who contain a little utility to Interns String
class UnboundTrace {

public:
  UnboundTrace(const std::string &output_path, uint64_t track_offset = 0);
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
    const auto [it, inserted] =
        name_to_iid_.try_emplace(name, static_cast<uint64_t>(name_to_iid_.size()) + 1);
    const uint64_t iid = it->second;

    if (inserted) {
      auto *interned_data = packet->mutable_interned_data();
      auto *entry = interned_data->add_event_names();
      entry->set_iid(iid);
      entry->set_name(name);
    }
    return iid;
  }

  template <typename... KeyArgs>
  Track &get_track(std::function<std::vector<std::string>(void)> get_names,
                   const std::tuple<KeyArgs...> &caching_key,
                   bool is_leaf_counter = false);

  template <typename... KeyArgs>
  void add_event_slice(const std::string &name,
                       uint64_t begin,
                       uint64_t end,
                       std::function<std::vector<std::string>(void)> get_names,
                       const std::tuple<KeyArgs...> &caching_key);

  template <typename... KeyArgs>
  void add_event_counter(uint64_t timestamp,
                         double value,
                         std::function<std::vector<std::string>(void)> get_names,
                         const std::tuple<KeyArgs...> &caching_key);

  uint64_t track_count = 1; // The Track 0 is reserved for Perfetto UI

private:
  static constexpr unsigned MAX_EVENT_PER_TRACE_CHUNK = 100'000;
  std::string output_path_;
  ::perfetto_pruned::Trace trace_{};
  std::unordered_map<std::string, uint64_t> name_to_iid_;
  unsigned current_packet_count_ = 0;
  std::unique_ptr<Track> track_ptr;

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
  static Track make_root(UnboundTrace *trace) { return Track(trace); }

  void add_event_slice(const std::string &name,
                       uint64_t begin,
                       uint64_t end,
                       std::optional<uint64_t> correlation_id = std::nullopt) {

    const auto uuid = get_slice_uuid(begin, end);
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

      if (correlation_id)
        track_event->set_correlation_id(correlation_id.value());
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

  void add_event_counter(uint64_t timestamp, double value) {

    const auto uuid = uuid_.value();
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

  // Look up in `childrens_` to and return you a track
  inline std::shared_ptr<Track> get_child(const std::string &name, bool is_leaf_counter = false) {
    // We need to avoid creating tmp Track when doing lookup.
    // inorder to avoid increase the `uuid` each time
    auto [it, inserted] = childrens_.try_emplace(
        name,
        nullptr // placeholder for unique_ptr; we will construct Track only if inserted
    );

    if (inserted) {
      // Construct the Track in-place and assign to the unique_ptr
      it->second = std::shared_ptr<Track>(new Track(name, uuid_, trace_ptr_, is_leaf_counter));
    } else if (it->second->is_leaf_counter_ != is_leaf_counter) {
      // Existing child but type mismatch
      throw std::invalid_argument("Asked for a type (counter or slice) got something else");
    }

    return it->second;
  }

private:
  Track(UnboundTrace *trace_ptr) : name_("root"), trace_ptr_(trace_ptr), is_leaf_counter_(false) {}

  Track(std::string name,
        std::optional<uint64_t> parent_uuid,
        UnboundTrace *trace_ptr,
        bool is_leaf_counter = false)
      : name_(std::move(name)), trace_ptr_(trace_ptr), uuid_(trace_ptr_->track_count++),
        parent_uuid_(parent_uuid), is_leaf_counter_(is_leaf_counter) {

    auto *packet = trace_ptr_->add_packet();
    packet->set_trusted_packet_sequence_id(TRUSTED_PACKED_SEQUENCE_ID);
    auto *td = packet->mutable_track_descriptor();

    td->set_name(name_);
    if (name_.rfind("Thread", 0) == 0)
      td->set_description("Host API activity. Expand (⌄) to view nested device tracks");

    td->set_uuid(uuid_.value());
    if (parent_uuid_)
      td->set_parent_uuid(parent_uuid_.value());

    if (is_leaf_counter_)
      td->mutable_counter();
  }

  std::string name_;
  UnboundTrace *trace_ptr_;
  // Root Have no UUID
  std::optional<uint64_t> uuid_;
  // First children have no parent_uuid (as root have no uuid)
  std::optional<uint64_t> parent_uuid_;
  // Leaf Counter Track need to set some special parameter to protobuf
  bool is_leaf_counter_;
  // Children are empty Track or counter Track
  // Need to use a pointer because unordered_map cannot use an imcompolete type
  // (https://stackoverflow.com/a/13089641)
  std::unordered_map<std::string, std::shared_ptr<Track>> childrens_;
  // Slice begins, they can be non perfectly nested, so generate a new track if required
  std::map<uint64_t, Track> begins_;

  // Lookup begins_ and return a correct `uuid` track
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
    }

    // Move the `uuid` who started before this events to the end
    auto itp = std::prev(it);
    // The doc said extract is the only way to change a key of a map element
    // without reallocation: https://en.cppreference.com/w/cpp/container/map/extract.html
    auto node = begins_.extract(itp);
    node.key() = end;
    auto result = begins_.insert(std::move(node));
    return (result.position->second.uuid_).value();
  }
};

UnboundTrace::UnboundTrace(const std::string &output_path, uint64_t track_offset)
    : output_path_(output_path), track_ptr(std::make_unique<Track>(Track::make_root(this))) {
  // Overwrite previous output_path file if existing
  std::ofstream(output_path_, std::ios::trunc | std::ios::binary);
  std::cout << "THAPI: Perfetto trace location: " << output_path_ << std::endl;
  track_count += track_offset;
}

// If you create a new instance of Trace, be carefull for the cache.
//  the key should take some `Trace Instance uuid`.
template <typename... KeyArgs>
inline Track &UnboundTrace::get_track(std::function<std::vector<std::string>(void)> get_names,
                                      const std::tuple<KeyArgs...> &caching_key,
                                      bool is_leaf_counter) {

  static std::unordered_map<std::tuple<KeyArgs...>, std::shared_ptr<Track>> cache;

  // Try the cache
  auto [it, inserted] = cache.try_emplace(caching_key, nullptr);
  if (!inserted)
    return *it->second;

  Track *t = track_ptr.get();

  // Ok then, generate the string, add do the lookup
  const auto names = get_names();
  if (names.empty())
    return *t;

  // Iterate over all except the last
  for (size_t i = 0; i < names.size() - 1; i++)
    t = t->get_child(names[i]).get();

  // The leaf will need to respected `is_leaf_counter`, and save the output
  it->second = t->get_child(names.back(), is_leaf_counter);

  // Return the now the pointer
  return *it->second;
}

template <typename... KeyArgs>
void UnboundTrace::add_event_slice(const std::string &name,
                                   uint64_t begin,
                                   uint64_t end,
                                   std::function<std::vector<std::string>(void)> get_names,
                                   const std::tuple<KeyArgs...> &caching_key) {

  get_track(get_names, caching_key).add_event_slice(name, begin, end);
}

template <typename... KeyArgs>
void UnboundTrace::add_event_counter(uint64_t timestamp,
                                     double value,
                                     std::function<std::vector<std::string>(void)> get_names,
                                     const std::tuple<KeyArgs...> &caching_key) {

  get_track(get_names, caching_key, true).add_event_counter(timestamp, value);
}

struct timeline_dispatch_s {
  std::unique_ptr<UnboundTrace> trace;
};

using timeline_dispatch_t = struct timeline_dispatch_s;

void btx_initialize_component_callback(void **usr_data) { *usr_data = new timeline_dispatch_t; }

static void read_params_callback(void *usr_data, btx_params_t *usr_params) {
  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);
  std::string output_path{usr_params->output_path};
  dispatch->trace = std::make_unique<UnboundTrace>(output_path, usr_params->offset);
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

  dispatch->trace->add_event_slice(
      name, ts, ts + dur,
      [=] {
        return std::vector{"Hostname " + std::string{hostname}, "Process " + std::to_string(vpid),
                           "Thread " + std::to_string(vtid)};
      },
      std::make_tuple(hostname, vpid, vtid));
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

  dispatch->trace->add_event_slice(
      name, ts, ts + dur,
      [=] {
        return std::vector{"Hostname " + std::string{hostname}, "Process " + std::to_string(vpid),
                           "Thread " + std::to_string(vtid), "Device " + std::to_string(did),
                           "SubDevice " + std::to_string(sdid)};
      },
      std::make_tuple(hostname, vpid, vtid, sdid));
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

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(frequency),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(domain),
                                        "Frequency"};
      },
      std::make_tuple(hostname, did, domain, "Frequency"));
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
    dispatch->trace->add_event_counter(
        ts, static_cast<double>(power),
        [=] {
          return std::vector<std::string>{"Hostname " + std::string{hostname}, "Sampling", "ZE",
                                          "Device " + std::to_string(did), "Power"};
        },
        std::make_tuple(hostname, did, "Power"));
  } else {
    dispatch->trace->add_event_counter(
        ts, static_cast<double>(power),
        [=] {
          return std::vector<std::string>{"Hostname " + std::string{hostname},
                                          "Sampling",
                                          "ZE",
                                          "Device " + std::to_string(did),
                                          "SubDevice " + std::to_string(domain - 1),
                                          "Power"};
        },
        std::make_tuple(hostname, did, domain - 1, "Power"));
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

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(activeTime),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        "ComputeEngine (%)"};
      },
      std::make_tuple(hostname, did, subDevice, "ComputeEngine (%)"));
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

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(activeTime),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        "CopyEngine (%)"};
      },
      std::make_tuple(hostname, did, subDevice, "CopyEngine (%)"));
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

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(rxThroughput),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        std::to_string(fabricId) + " <->" +
                                            std::to_string(remotePortId),
                                        "RX"};
      },
      std::make_tuple(hostname, did, subDevice, fabricId, remotePortId, "RX"));

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(txThroughput),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        std::to_string(fabricId) + " <->" +
                                            std::to_string(remotePortId),
                                        "TX"};
      },
      std::make_tuple(hostname, did, subDevice, fabricId, remotePortId, "TX"));
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

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(pBandwidth),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        "Memory BW"};
      },
      std::make_tuple(hostname, did, subDevice, "Memory BW"));

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(allocation),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname},
                                        "Sampling",
                                        "ZE",
                                        "Device " + std::to_string(did),
                                        "SubDevice " + std::to_string(subDevice),
                                        "Allocated Memory (%)"};
      },
      std::make_tuple(hostname, did, subDevice, "Allocated Memory (%)"));
}

static void nic_usr_callback(void *btx_handle,
                             void *usr_data,
                             const char *hostname,
                             int64_t ts,
                             const char *interface_name,
                             const char *counter,
                             uint64_t value) {

  auto *dispatch = static_cast<timeline_dispatch_t *>(usr_data);

  dispatch->trace->add_event_counter(
      ts, static_cast<double>(value),
      [=] {
        return std::vector<std::string>{"Hostname " + std::string{hostname}, "Sampling", "NIC",
                                        "Interface " + std::string{interface_name}, counter};
      },
      std::make_tuple(hostname, interface_name, counter, "NIC"));
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
