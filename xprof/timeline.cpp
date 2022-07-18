#include "timeline.h"
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
                            timestamp_t begin, std::string name, std::set<flow_id_t> flow_ids = {} ) {
  auto *packet = dispatch->trace.add_packet();
  packet->set_timestamp(begin);
  packet->set_trusted_packet_sequence_id(10);
  auto *track_event = packet->mutable_track_event();
  track_event->set_type(perfetto_pruned::TrackEvent::TYPE_SLICE_BEGIN);
  track_event->set_name(name);
  track_event->set_track_uuid(uuid);
  for (const auto& flow_id: flow_ids)
    track_event->add_flow_ids(flow_id);
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
                          uint64_t dur, std::set<flow_id_t> flow_ids = {}) {
  // Assume perfecly nessted
  const uint64_t end = begin + dur;

  perfetto_uuid_t parent_uuid = get_parent_uuid(dispatch, hostname, process_id, thread_id);
  // Handling perfecly nested event
  add_event_begin(dispatch, parent_uuid, begin, name, flow_ids);
  std::stack<uint64_t> &s = dispatch->uuid2stack[parent_uuid];
  while ((!s.empty()) && (s.top() <= begin)) {
    add_event_end(dispatch, parent_uuid, s.top());
    s.pop();
  }
  s.push(end);
}

static void add_event_gpu_old(struct timeline_dispatch *dispatch, std::string hostname,
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

static void add_event_gpu(struct timeline_dispatch *dispatch, std::string hostname,
                          uint64_t process_id, uint64_t c_uuid, std::string queue_name,
                          thapi_device_id did, thapi_device_id sdid,
                          std::string name, uint64_t begin, uint64_t dur, std::set<flow_id_t> flow_ids) {
  // This function Assume non perfecly nested
  const uint64_t end = begin + dur;
  perfetto_uuid_t parent_uuid = get_parent_uuid(dispatch, hostname, process_id, c_uuid, did, sdid);
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
      oss << queue_name << " " << c_uuid;
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
  add_event_begin(dispatch, uuid, begin, name, flow_ids);
  add_event_end(dispatch, uuid, end);
}

bt_component_class_sink_consume_method_status
timeline_dispatch_consume(bt_self_component_sink *self_component_sink) {
  bt_component_class_sink_consume_method_status status =
      BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;

  // Internal datatrastruct to convern hostname process to Google trace format process
  // https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview

  /* Retrieve our private data from the component's user data */
  struct timeline_dispatch *dispatch = (timeline_dispatch *)bt_self_component_get_data(
      bt_self_component_sink_as_self_component(self_component_sink));

  /* Consume a batch of messages from the upstream message iterator */
  bt_message_array_const messages;
  uint64_t message_count;
  bt_message_iterator_next_status next_status =
      bt_message_iterator_next(dispatch->message_iterator, &messages, &message_count);

  switch (next_status) {
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_END:
    /* End of iteration: put the message iterator's reference */
    bt_message_iterator_put_ref(dispatch->message_iterator);
    status = BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_END;
    goto end;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_AGAIN:
    status = BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_AGAIN;
    goto end;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_MEMORY_ERROR:
    status = BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_MEMORY_ERROR;
    goto end;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_ERROR:
    status = BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_ERROR;
    goto end;
  default:
    break;
  }

  /* For each consumed message */
  for (uint64_t i = 0; i < message_count; i++) {
    const bt_message *message = messages[i];
    if (bt_message_get_type(message) == BT_MESSAGE_TYPE_EVENT) {
      const bt_event *event = bt_message_event_borrow_event_const(message);
      const bt_event_class *event_class = bt_event_borrow_class_const(event);
      const char *class_name = bt_event_class_get_name(event_class);

      auto dur_tuple0 =
          std::make_tuple(std::make_tuple(0, &bt_field_string_get_value,
                                          (hostname_t) ""), // hostname
                          std::make_tuple(1, &bt_field_integer_signed_get_value,
                                          (process_id_t)0), // process
                          std::make_tuple(2, &bt_field_integer_unsigned_get_value,
                                          (thread_id_t)0), // thread
                          std::make_tuple(3, &bt_field_integer_unsigned_get_value, (uint64_t)0));

      const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);
      const auto & [ hostname, process_id, thread_id, ts ] =
          thapi_bt2_getter(common_context_field, dur_tuple0);

      const bt_field *payload_field = bt_event_borrow_payload_field_const(event);

      const bt_field *name_field =
          bt_field_structure_borrow_member_field_by_index_const(payload_field, 0);
      const std::string name = std::string{bt_field_string_get_value(name_field)};

      const bt_field *dur_field =
          bt_field_structure_borrow_member_field_by_index_const(payload_field, 1);
      const long dur = bt_field_integer_unsigned_get_value(dur_field);

      if (std::string(class_name) == "lttng:host") {
        add_event_cpu(dispatch, hostname, process_id, thread_id, name, ts, dur);
      } else if (std::string(class_name) == "lttng:host_flow") {

        const bt_field *flow_ids_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 4);

        std::set<flow_id_t> flow_ids;
        for (unsigned i=0; i < bt_field_array_get_length(flow_ids_field); i++) {
            const bt_field *flow_id_field = bt_field_array_borrow_element_field_by_index_const(flow_ids_field, i);
            flow_ids.insert(bt_field_integer_unsigned_get_value(flow_id_field));
        }
        add_event_cpu(dispatch, hostname, process_id, thread_id, name, ts, dur, flow_ids);
      } else if (std::string(class_name) == "lttng:device") {

        const bt_field *did_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 2);
        const thapi_device_id did = bt_field_integer_unsigned_get_value(did_field);

        const bt_field *sdid_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 3);
        const thapi_device_id sdid = bt_field_integer_unsigned_get_value(sdid_field);
        add_event_gpu_old(dispatch, hostname, process_id, thread_id, did, sdid, name, ts, dur);
      } else if (std::string(class_name) == "lttng:device_flow") {

        const bt_field *did_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 2);
        const thapi_device_id did = bt_field_integer_unsigned_get_value(did_field);

        const bt_field *sdid_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 3);
        const thapi_device_id sdid = bt_field_integer_unsigned_get_value(sdid_field);

        const bt_field *uuid_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 6);
        const long uuid = bt_field_integer_unsigned_get_value(uuid_field);

        const bt_field *queue_name_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 7);
        const std::string queue_name = bt_field_string_get_value(queue_name_field);

        const bt_field *flow_id_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field, 8);
        const uint64_t flow_id = bt_field_integer_unsigned_get_value(flow_id_field);


        add_event_gpu(dispatch, hostname, process_id, uuid, queue_name, did, sdid, name, ts, dur, {flow_id} );
      }

    }
    bt_message_put_ref(message);
  }
end:
  return status;
}

/*
 * Initializes the sink component.
 */
bt_component_class_initialize_method_status
timeline_dispatch_initialize(bt_self_component_sink *self_component_sink,
                             bt_self_component_sink_configuration *configuration,
                             const bt_value *params, void *initialize_method_data) {
  /* Allocate a private data structure */
  struct timeline_dispatch *dispatch = new timeline_dispatch;

  /* Set the component's user data to our private data structure */
  bt_self_component_set_data(bt_self_component_sink_as_self_component(self_component_sink),
                             dispatch);

  /*
   * Add an input port named `in` to the sink component.
   *
   * This is needed so that this sink component can be connected to a
   * filter or a source component. With a connected upstream
   * component, this sink component can create a message iterator
   * to consume messages.
   */
  bt_self_component_sink_add_input_port(self_component_sink, "in", NULL, NULL);

  {
    auto *packet = dispatch->trace.add_packet();
    packet->set_trusted_packet_sequence_id(10);
    packet->set_timestamp(0);

    auto *trace_packet_defaults = packet->mutable_trace_packet_defaults();
    trace_packet_defaults->set_timestamp_clock_id(perfetto_pruned::BUILTIN_CLOCK_BOOTTIME);
    packet->set_previous_packet_dropped(true);
  }
  return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the sink component.
 */
void timeline_dispatch_finalize(bt_self_component_sink *self_component_sink) {
  struct timeline_dispatch *dispatch = (timeline_dispatch *)bt_self_component_get_data(
      bt_self_component_sink_as_self_component(self_component_sink));

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
}

/*
 * Called when the trace processing graph containing the sink component
 * is configured.
 *
 * This is where we can create our upstream message iterator.
 */
bt_component_class_sink_graph_is_configured_method_status
timeline_dispatch_graph_is_configured(bt_self_component_sink *self_component_sink) {
  /* Retrieve our private data from the component's user data */
  struct timeline_dispatch *dispatch = (timeline_dispatch *)bt_self_component_get_data(
      bt_self_component_sink_as_self_component(self_component_sink));

  /* Borrow our unique port */
  bt_self_component_port_input *in_port =
      bt_self_component_sink_borrow_input_port_by_index(self_component_sink, 0);

  /* Create the uptream message iterator */
  bt_message_iterator_create_from_sink_component(self_component_sink, in_port,
                                                 &dispatch->message_iterator);

  return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}
