#include <metababel/metababel.h>

#include "timeline.hpp"
#include "xprof_utils.hpp" // typedef
#include <fstream>
#include <iomanip>  // set precision
#include <iostream> // stdcout
#include <stack>

#include "perfetto_prunned.pb.h"

static perfetto_uuid_t gen_perfetto_uuid() {
  // Start at one, Look like UUID 0 is special
  static std::atomic<perfetto_uuid_t> uuid{1};
  return uuid++;
}

static void add_event_begin(struct timeline_dispatch *dispatch, perfetto_uuid_t uuid,
                            timestamp_t begin, std::string name) {
  auto *packet = dispatch->trace.add_packet();
  packet->set_timestamp(begin);
  packet->set_trusted_packet_sequence_id(10);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_BEGIN);
  track_event->set_name(name);
  track_event->set_track_uuid(uuid);
}

static void add_event_end(struct timeline_dispatch *dispatch, perfetto_uuid_t uuid, uint64_t end) {
  auto *packet = dispatch->trace.add_packet();
  packet->set_trusted_packet_sequence_id(10);
  packet->set_timestamp(end);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_END);
  track_event->set_track_uuid(uuid);
}

static perfetto_uuid_t get_parent_uuid(struct timeline_dispatch *dispatch, std::string hostname,
                                       uint64_t process_id, uint64_t thread_id,
                                       thapi_device_id did = 0, thapi_device_id sdid = 0) {

  perfetto_uuid_t hp_uuid = 0;
  {
    // This is so easy...
    // Because element keys in a map are unique,
    // the insertion operation checks whether each inserted element has a key equivalent to the one
    // of an element already in the container, and if so, the element is not inserted, returning an
    // iterator to this existing element (if the function returns a value).

    // In the case we where not able to insert, we use the iterator to get the value,
    auto r = dispatch->hp2uuid.insert({{hostname, process_id, did, sdid}, hp_uuid});
    auto &potential_uuid = r.first->second;
    if (!r.second) {
      hp_uuid = potential_uuid;
      // In the case we where able to insert our dummy value,
      // We generate the a new uuid, and mutate the value in the map
    } else {
      hp_uuid = gen_perfetto_uuid();
      potential_uuid = hp_uuid;

      {
        auto *packet = dispatch->trace.add_packet();
        packet->set_trusted_packet_sequence_id(10);
        packet->set_timestamp(0);

        auto *track_descriptor = packet->mutable_track_descriptor();
        track_descriptor->set_uuid(hp_uuid);
        auto *process = track_descriptor->mutable_process();
        process->set_pid(hp_uuid);
        std::ostringstream oss;
        oss << hostname << " | Process " << process_id ;
        if (did !=0) {
            oss << " | Device " << did;
            if (sdid !=0)
                oss << " | SubDevice " << sdid;
        }
        oss << " | uuid ";
        process->set_process_name(oss.str());
      }
    }
  }
  // Due to Perfetto https://github.com/google/perfetto/issues/321,
  // each GPU thread wil be mapped to a "virtual" process
  // We will add the thread_id to the process name
  perfetto_uuid_t parent_uuid = 0;
  {
    // Same strategy used previsouly to do only one table lookup
    auto r = dispatch->hpt2uuid.insert({{hp_uuid, thread_id}, parent_uuid});
    auto &potential_uuid = r.first->second;
    if (!r.second) {
      parent_uuid = potential_uuid;
    } else {
      parent_uuid = gen_perfetto_uuid();
      potential_uuid = parent_uuid;
      {
        auto *packet = dispatch->trace.add_packet();
        packet->set_trusted_packet_sequence_id(10);
        packet->set_timestamp(0);

        auto *track_descriptor = packet->mutable_track_descriptor();
        track_descriptor->set_uuid(parent_uuid);
        track_descriptor->set_parent_uuid(hp_uuid);
        // This is the workarround for the bug: https://github.com/google/perfetto/issues/321
        //   We trick perfetto to this they are processes
        if (did == 0) {
          auto *thread = track_descriptor->mutable_thread();
          thread->set_pid(hp_uuid);
          thread->set_tid(thread_id);
        }
      }
    }
  }
  return parent_uuid;
}

static void add_event_cpu(struct timeline_dispatch *dispatch, std::string hostname,
                          uint64_t process_id, uint64_t thread_id, std::string name, uint64_t begin,
                          uint64_t dur) {
  // Assume perfecly nessted
  const uint64_t end = begin + dur;

  perfetto_uuid_t parent_uuid = get_parent_uuid(dispatch, hostname, process_id, thread_id);
  // Handling perfecly nested event
  add_event_begin(dispatch, parent_uuid, begin, name);
  std::stack<uint64_t> &s = dispatch->uuid2stack[parent_uuid];
  while ((!s.empty()) && (s.top() <= begin)) {
    add_event_end(dispatch, parent_uuid, s.top());
    s.pop();
  }
  s.push(end);
}

static void add_event_gpu(struct timeline_dispatch *dispatch, std::string hostname,
                          uint64_t process_id, uint64_t thread_id, thapi_device_id did,
                          thapi_device_id sdid, std::string name, uint64_t begin, uint64_t dur) {
  // This function Assume non perfecly nested
  const uint64_t end = begin + dur;
  perfetto_uuid_t parent_uuid = get_parent_uuid(dispatch, hostname, process_id, thread_id, did, sdid);
  // Now see if we need a to generate a new children
  std::map<uint64_t, perfetto_uuid_t> &m = dispatch->parents2tracks[parent_uuid];
  perfetto_uuid_t uuid;

  // Pre-historical event
  if (m.empty() || begin < m.begin()->first) {
    uuid = gen_perfetto_uuid();
    // Generate a new children track
    {
      auto *packet = dispatch->trace.add_packet();
      packet->set_trusted_packet_sequence_id(10);
      packet->set_timestamp(0);

      auto *track_descriptor = packet->mutable_track_descriptor();
      track_descriptor->set_uuid(uuid);
      track_descriptor->set_parent_uuid(parent_uuid);

      std::ostringstream oss;
      oss << "Thread " << thread_id;
      track_descriptor->set_name(oss.str());
    }
  } else {
    // Find the uuid who finished just before this one
    auto it_ub = std::prev(m.upper_bound(begin));
    uuid = it_ub->second;
    // Erase the old timestamps
    m.erase(it_ub);
  }
  // Update the table
  m[end] = uuid;
  // Add event
  add_event_begin(dispatch, uuid, begin, name);
  add_event_end(dispatch, uuid, end);
}

void btx_initialize_usr_data(void *btx_handle, void **usr_data){
  *usr_data = new timeline_dispatch_t;

  auto *packet = ((timeline_dispatch_t *)(*usr_data))->trace.add_packet();
  packet->set_trusted_packet_sequence_id(10);
  packet->set_timestamp(0);

  auto *trace_packet_defaults = packet->mutable_trace_packet_defaults();
  trace_packet_defaults->set_timestamp_clock_id(perfetto_pruned::BUILTIN_CLOCK_BOOTTIME);
  packet->set_previous_packet_dropped(true);
}

void btx_finalize_usr_data(void *btx_handle, void *usr_data){
  timeline_dispatch_t *dispatch = (timeline_dispatch_t *)usr_data;

  for (auto & [ uuid, s ] : dispatch->uuid2stack) {
    while (!s.empty()) {
      add_event_end(dispatch, uuid, s.top());
      s.pop();
    }
  }
  std::string path{"out.pftrace"};
  // Write the new address book back to disk.
  std::fstream output(path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!dispatch->trace.SerializeToOstream(&output))
    std::cerr << "Failed to write the trace." << std::endl;
  else
    std::cout << "Perfetto trace saved: " << path << std::endl;
  google::protobuf::ShutdownProtobufLibrary();

  free(dispatch);
}

static void lttng_host_usr_callback(
    void *btx_handle, void *usr_data,   const char* hostname,
    int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend_id, const char* name,
    uint64_t dur, bt_bool err
)
{
  timeline_dispatch_t *dispatch = (timeline_dispatch_t *)usr_data;
  add_event_cpu(dispatch, hostname, vpid, vtid, name, ts, dur);
}

static void lttng_device_usr_callback(
    void *btx_handle, void *usr_data, const char* hostname, int64_t vpid,
    uint64_t vtid, int64_t ts, int64_t backend, const char* name, uint64_t dur, 
    uint64_t did, uint64_t sdid, bt_bool err, const char* metadata
)
{
  timeline_dispatch_t *dispatch = (timeline_dispatch_t *)usr_data;
  add_event_gpu(dispatch, hostname, vpid, vtid, did, sdid, name, ts, dur);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_lttng_host(btx_handle, &lttng_host_usr_callback);
  btx_register_callbacks_lttng_device(btx_handle, &lttng_device_usr_callback);
  btx_register_callbacks_initialize_usr_data(btx_handle,&btx_initialize_usr_data);
  btx_register_callbacks_finalize_usr_data(btx_handle, &btx_finalize_usr_data);
}
