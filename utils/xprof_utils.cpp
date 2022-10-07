#include "xprof_utils.hpp"
#include <set>

const char* borrow_hostname(const bt_event *event){
    const bt_stream *stream = bt_event_borrow_stream_const(event);
    const bt_trace *trace = bt_stream_borrow_trace_const(stream);
    const bt_value *host_name_str = bt_trace_borrow_environment_entry_value_by_name_const(trace, "hostname");
    return  bt_value_string_get(host_name_str);
}

process_id_t borrow_process_id(const bt_event *event){
    const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);
    const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 0);
    return bt_field_integer_signed_get_value(field);
}


thread_id_t borrow_thread_id(const bt_event *event){
    const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);
    const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 1);
    return bt_field_integer_unsigned_get_value(field);
}

bt_message* create_host_message(const char* hostname, const process_id_t process_id, const thread_id_t thread_id, const char* name,
        const uint64_t ts, const uint64_t duration, const bool err,
        bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend, std::set<flow_id_t> flow_ids) {

     /* Message creation */
     bt_message *message = bt_message_event_create(
                             message_iterator, event_class, stream);

     /* event */
     bt_event *downstream_event = bt_message_event_borrow_event(message);

     /* Common context */
     bt_field *context_field = bt_event_borrow_common_context_field(downstream_event);

     // Hostname
     bt_field *hostname_msg_field = bt_field_structure_borrow_member_field_by_index(context_field,0);
     bt_field_string_set_value(hostname_msg_field, hostname);
     // pid
     bt_field *vpid_field = bt_field_structure_borrow_member_field_by_index(context_field,1);
     bt_field_integer_signed_set_value(vpid_field, process_id);
     // vid
     bt_field *vtid_field = bt_field_structure_borrow_member_field_by_index(context_field,2);
     bt_field_integer_signed_set_value(vtid_field, thread_id);
     // ts
     bt_field *ts_field = bt_field_structure_borrow_member_field_by_index(context_field,3);
     bt_field_integer_signed_set_value(ts_field, ts);
     // backend
     bt_field *backend_field = bt_field_structure_borrow_member_field_by_index(context_field,4);
     bt_field_integer_signed_set_value(backend_field, backend);

     /* Payload */
     bt_field *payload_field = bt_event_borrow_payload_field(downstream_event);

     // name
     bt_field *name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 0);
     bt_field_string_set_value(name_field, name);

     // dur
     bt_field *dur_field = bt_field_structure_borrow_member_field_by_index(payload_field, 1);
     bt_field_integer_unsigned_set_value(dur_field, duration);

     // err
     bt_field *err_field = bt_field_structure_borrow_member_field_by_index(payload_field, 2);
     bt_field_integer_unsigned_set_value(err_field, err);

     // flow id
     if (!flow_ids.empty() ) {
        const auto size = flow_ids.size();
        //bt_field *flow_id_field_length = bt_field_structure_borrow_member_field_by_index(payload_field, 3);
        //bt_field_integer_unsigned_set_value(flow_id_field_length, size);

        bt_field *flow_id_field = bt_field_structure_borrow_member_field_by_index(payload_field, 4);
        bt_field_array_dynamic_set_length(flow_id_field, size);

        int i=0;
        for (auto element: flow_ids) {
            bt_field *element_field = bt_field_structure_borrow_member_field_by_index(flow_id_field, i++);
            bt_field_integer_unsigned_set_value(element_field, element);
        }
     }
     return message;
}


bt_message* create_device_message(const char* hostname, const process_id_t process_id, const thread_id_t thread_id,
                                  const thapi_device_id device_id, const thapi_device_id subdevice_id,
                                  const char* name, const uint64_t ts, const uint64_t duration, const bool err,
                                  const char* metadata,
                                  bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream) {

     /* Message creation */
     bt_message *message = bt_message_event_create(
                             message_iterator, event_class, stream);


     /* event */
     bt_event *downstream_event = bt_message_event_borrow_event(message);

     /* Common context */
     bt_field *context_field = bt_event_borrow_common_context_field(downstream_event);

     // Hostname
     bt_field *hostname_msg_field = bt_field_structure_borrow_member_field_by_index(context_field,0);
     bt_field_string_set_value(hostname_msg_field, hostname);
     // pid
     bt_field *vpid_field = bt_field_structure_borrow_member_field_by_index(context_field,1);
     bt_field_integer_signed_set_value(vpid_field, process_id);
     // vid
     bt_field *vtid_field = bt_field_structure_borrow_member_field_by_index(context_field,2);
     bt_field_integer_signed_set_value(vtid_field, thread_id);
     // ts
     bt_field *ts_field = bt_field_structure_borrow_member_field_by_index(context_field,3);
     bt_field_integer_signed_set_value(ts_field, ts);

     /* Payload */
     bt_field *payload_field = bt_event_borrow_payload_field(downstream_event);

     // name
     bt_field *name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 0);
     bt_field_string_set_value(name_field, name);

     // dur
     bt_field *dur_field = bt_field_structure_borrow_member_field_by_index(payload_field, 1);
     bt_field_integer_unsigned_set_value(dur_field, duration);

     // did
     bt_field *device_id_field = bt_field_structure_borrow_member_field_by_index(payload_field,2);
     bt_field_integer_unsigned_set_value(device_id_field, device_id);

     // sdid
     bt_field *subdevice_id_field = bt_field_structure_borrow_member_field_by_index(payload_field,3);
     bt_field_integer_unsigned_set_value(subdevice_id_field, subdevice_id);

     // err
     bt_field *err_field = bt_field_structure_borrow_member_field_by_index(payload_field, 4);
     bt_field_integer_unsigned_set_value(err_field, err);

     //Metadata
     bt_field *metadata_field = bt_field_structure_borrow_member_field_by_index(payload_field, 5);
     bt_field_string_set_value(metadata_field, metadata);
     return message;
}

bt_message* create_device_flow_message(const char* hostname, const process_id_t process_id, const uint64_t uuid,
                                  const thapi_device_id device_id, const thapi_device_id subdevice_id,
                                  const char* name, const uint64_t ts, const uint64_t duration, const bool err,
                                  const char* metadata, const char* queue_name, const flow_id_t flow_id,
                                  bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream) {

     /* Message creation */
     bt_message *message = bt_message_event_create(
                             message_iterator, event_class, stream);


     /* event */
     bt_event *downstream_event = bt_message_event_borrow_event(message);

     /* Common context */
     bt_field *context_field = bt_event_borrow_common_context_field(downstream_event);

     // Hostname
     bt_field *hostname_msg_field = bt_field_structure_borrow_member_field_by_index(context_field,0);
     bt_field_string_set_value(hostname_msg_field, hostname);
     // pid
     bt_field *vpid_field = bt_field_structure_borrow_member_field_by_index(context_field,1);
     bt_field_integer_signed_set_value(vpid_field, process_id);
     // vid
     bt_field *vtid_field = bt_field_structure_borrow_member_field_by_index(context_field,2);
     bt_field_integer_signed_set_value(vtid_field, 0);
     // ts
     bt_field *ts_field = bt_field_structure_borrow_member_field_by_index(context_field,3);
     bt_field_integer_signed_set_value(ts_field, ts);

     /* Payload */
     bt_field *payload_field = bt_event_borrow_payload_field(downstream_event);

     // name
     bt_field *name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 0);
     bt_field_string_set_value(name_field, name);

     // dur
     bt_field *dur_field = bt_field_structure_borrow_member_field_by_index(payload_field, 1);
     bt_field_integer_unsigned_set_value(dur_field, duration);

     // did
     bt_field *device_id_field = bt_field_structure_borrow_member_field_by_index(payload_field,2);
     bt_field_integer_unsigned_set_value(device_id_field, device_id);

     // sdid
     bt_field *subdevice_id_field = bt_field_structure_borrow_member_field_by_index(payload_field,3);
     bt_field_integer_unsigned_set_value(subdevice_id_field, subdevice_id);

     // err
     bt_field *err_field = bt_field_structure_borrow_member_field_by_index(payload_field, 4);
     bt_field_integer_unsigned_set_value(err_field, err);

     //Metadata
     bt_field *metadata_field = bt_field_structure_borrow_member_field_by_index(payload_field, 5);
     bt_field_string_set_value(metadata_field, metadata);

     // uuid
     bt_field *uuid_field = bt_field_structure_borrow_member_field_by_index(payload_field, 6);
     bt_field_integer_unsigned_set_value(uuid_field, uuid);

     //Queue_name
     bt_field *queue_name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 7);
     bt_field_string_set_value(queue_name_field, queue_name);

     // flow_id
     bt_field *flow_field = bt_field_structure_borrow_member_field_by_index(payload_field, 8);
     bt_field_integer_unsigned_set_value(flow_field, flow_id);

     return message;
}

bt_message* create_device_name_message(const char* hostname, const process_id_t process_id,
                                       const thapi_device_id device_id, const char* name,
                                       bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream) {

     /* Message creation */
     bt_message *message = bt_message_event_create(
                             message_iterator, event_class, stream);


     /* event */
     bt_event *downstream_event = bt_message_event_borrow_event(message);

     /* Common context */
     bt_field *context_field = bt_event_borrow_common_context_field(downstream_event);

     // Hostname
     bt_field *hostname_msg_field = bt_field_structure_borrow_member_field_by_index(context_field,0);
     bt_field_string_set_value(hostname_msg_field, hostname);
     // pid
     bt_field *vpid_field = bt_field_structure_borrow_member_field_by_index(context_field,1);
     bt_field_integer_signed_set_value(vpid_field, process_id);

     /* Payload */
     bt_field *payload_field = bt_event_borrow_payload_field(downstream_event);

     // name
     bt_field *name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 0);
     bt_field_string_set_value(name_field, name);

     // did
     bt_field *device_id_field = bt_field_structure_borrow_member_field_by_index(payload_field,1);
     bt_field_integer_unsigned_set_value(device_id_field, device_id);

    return message;
}

bt_message* create_traffic_message(const char *hostname, const process_id_t process_id , const thread_id_t thread_id,
                                   const char *name, const uint64_t size,
                                   bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend) {

     /* Message creation */
     bt_message *message = bt_message_event_create(
                             message_iterator, event_class, stream);

     /* event */
     bt_event *downstream_event = bt_message_event_borrow_event(message);

     /* Common context */
     bt_field *context_field = bt_event_borrow_common_context_field(downstream_event);

     // Hostname
     bt_field *hostname_msg_field = bt_field_structure_borrow_member_field_by_index(context_field,0);
     bt_field_string_set_value(hostname_msg_field, hostname);
     // pid
     bt_field *vpid_field = bt_field_structure_borrow_member_field_by_index(context_field,1);
     bt_field_integer_signed_set_value(vpid_field, process_id);
     // vid
     bt_field *vtid_field = bt_field_structure_borrow_member_field_by_index(context_field,2);
     bt_field_integer_signed_set_value(vtid_field, thread_id);
     // backend
     bt_field *backend_field = bt_field_structure_borrow_member_field_by_index(context_field,3);
     bt_field_integer_signed_set_value(backend_field, backend);

     /* Payload */
     bt_field *payload_field = bt_event_borrow_payload_field(downstream_event);

     // name
     bt_field *name_field = bt_field_structure_borrow_member_field_by_index(payload_field, 0);
     bt_field_string_set_value(name_field, name);

     // size
     bt_field *dur_field = bt_field_structure_borrow_member_field_by_index(payload_field, 1);
     bt_field_integer_unsigned_set_value(dur_field, size);

     return message;
}

