#include "tally.h"
#include "tally.hpp"
#include "xprof_utils.hpp" //Typedef and hashtuple

static inline void get_common_lltng(const bt_event *event, hostname_t &hostname,
                                    process_id_t &process_id,
                                    thread_id_t &thread_id, std::string &name) {
  // Common context field
  const bt_field *common_context_field =
      bt_event_borrow_common_context_field_const(event);

  const bt_field *hostname_field =
      bt_field_structure_borrow_member_field_by_index_const(
          common_context_field, 0);
  hostname = std::string{bt_field_string_get_value(hostname_field)};

  const bt_field *process_id_field =
      bt_field_structure_borrow_member_field_by_index_const(
          common_context_field, 1);
  process_id = bt_field_integer_signed_get_value(process_id_field);

  const bt_field *thread_id_field =
      bt_field_structure_borrow_member_field_by_index_const(
          common_context_field, 2);
  thread_id = bt_field_integer_unsigned_get_value(thread_id_field);

  // Payload
  const bt_field *payload_field = bt_event_borrow_payload_field_const(event);

  const bt_field *name_field =
      bt_field_structure_borrow_member_field_by_index_const(payload_field, 0);
  name = std::string{bt_field_string_get_value(name_field)};
}

bt_component_class_sink_consume_method_status
tally_dispatch_consume(bt_self_component_sink *self_component_sink) {
  bt_component_class_sink_consume_method_status status =
      BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;

  /* Retrieve our private data from the component's user data */
  struct tally_dispatch *dispatch =
      (tally_dispatch *)bt_self_component_get_data(
          bt_self_component_sink_as_self_component(self_component_sink));

  /* Consume a batch of messages from the upstream message iterator */
  bt_message_array_const messages;
  uint64_t message_count;
  bt_message_iterator_next_status next_status = bt_message_iterator_next(
      dispatch->message_iterator, &messages, &message_count);

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

      hostname_t hostname;
      process_id_t process_id;
      thread_id_t thread_id;
      std::string name;

      const bt_field *payload_field =
          bt_event_borrow_payload_field_const(event);
      if (strcmp(class_name, "lttng:host") == 0) {
        get_common_lltng(event, hostname, process_id, thread_id, name);

        auto dur_tuple0 = std::make_tuple(
            std::make_tuple(1, &bt_field_integer_unsigned_get_value, (uint64_t) 0),
            std::make_tuple(2, &bt_field_bool_get_value, (bool) 0));

        const auto [dur, err] = thapi_bt2_getter(payload_field, dur_tuple0);

        TallyCoreTime a{dur, err};
        dispatch->host2[hpt_function_name_t(hostname, process_id, thread_id,
                                            (thapi_function_name)name)] += a;

      } else if (strcmp(class_name, "lttng:device") == 0) {
        get_common_lltng(event, hostname, process_id, thread_id, name);

        const bt_field *dur_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  1);
        const long dur = bt_field_integer_unsigned_get_value(dur_field);

        const bt_field *did_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  2);
        const thapi_device_id did =
            bt_field_integer_unsigned_get_value(did_field);

        const bt_field *sdid_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  3);
        const thapi_device_id sdid =
            bt_field_integer_unsigned_get_value(sdid_field);

        const bt_field *err_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  4);
        const bool err = bt_field_bool_get_value(err_field);

        if (dispatch->demangle_name)
          /*Should fucking cache this function */
          name = f_demangle_name(name);
        // Will add if statements
        // name = name + " ( [0,0,0])";
        // Should handle Datatransder, and kernel in different event

        TallyCoreTime a{(uint64_t)dur, err};
        dispatch->device2[hpt_device_function_name_t(
            hostname, process_id, thread_id, did, sdid,
            (thapi_function_name)name)] += a;

      } else if (strcmp(class_name, "lttng:traffic") == 0) {
        get_common_lltng(event, hostname, process_id, thread_id, name);

        const bt_field *size_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  1);
        const long size = bt_field_integer_unsigned_get_value(size_field);
        TallyCoreByte a{(uint64_t)size, false};
        dispatch->traffic2[hpt_function_name_t(hostname, process_id, thread_id,
                                               name)] += a;

      } else if (strcmp(class_name, "lttng:device_name") == 0) {
        get_common_lltng(event, hostname, process_id, thread_id, name);

        const bt_field *did_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  1);
        const thapi_device_id did =
            bt_field_integer_unsigned_get_value(did_field);

        dispatch->device_name[hp_device_t(hostname, process_id,
                                          (thapi_device_id)did)] = name;
      } else if (strcmp(class_name, "lttng_ust_thapi:metadata") == 0) {
        const bt_field *value_field =
            bt_field_structure_borrow_member_field_by_index_const(payload_field,
                                                                  0);
        dispatch->metadata.push_back(
            std::string{bt_field_string_get_value(value_field)});
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
tally_dispatch_initialize(bt_self_component_sink *self_component_sink,
                          bt_self_component_sink_configuration *configuration,
                          const bt_value *params,
                          void *initialize_method_data) {
  /*Read env variable */
  const bt_value *val;
  val = bt_value_map_borrow_entry_value_const(params, "display");
  const std::string display_mode(
      val && bt_value_is_string(val) ? bt_value_string_get(val) : "compact");
  val = bt_value_map_borrow_entry_value_const(params, "name");
  const std::string display_name(
      val && bt_value_is_string(val) ? bt_value_string_get(val) : "mangle");
  val = bt_value_map_borrow_entry_value_const(params, "display_mode");
  const std::string display_human(
      val && bt_value_is_string(val) ? bt_value_string_get(val) : "human");
  val = bt_value_map_borrow_entry_value_const(params, "display_metadata");
  const bool display_metadata =
      (val && bt_value_is_bool(val) ? bt_value_bool_get(val) : false);

  val = bt_value_map_borrow_entry_value_const(params, "display_name_max_size");
  const int display_name_max_size =
      (val && bt_value_is_signed_integer(val) ? bt_value_integer_signed_get(val)
                                              : -1);

  /* Allocate a private data structure */
  struct tally_dispatch *dispatch = new tally_dispatch;

  dispatch->display_compact =
      (display_mode == "compact");                        // Compact or Extented
  dispatch->demangle_name = (display_name == "demangle"); // Demangle or mangle
  dispatch->display_human = (display_human == "human");   // Human or JSON
  dispatch->display_metadata = display_metadata;
  dispatch->display_name_max_size = display_name_max_size;

  /* Set the component's user data to our private data structure */
  bt_self_component_set_data(
      bt_self_component_sink_as_self_component(self_component_sink), dispatch);

  /*
   * Add an input port named `in` to the sink component.
   *
   * This is needed so that this sink component can be connected to a
   * filter or a source component. With a connected upstream
   * component, this sink component can create a message iterator
   * to consume messages.
   */
  bt_self_component_sink_add_input_port(self_component_sink, "in", NULL, NULL);

  return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the sink component.
 */
void tally_dispatch_finalize(bt_self_component_sink *self_component_sink) {
  struct tally_dispatch *dispatch =
      (tally_dispatch *)bt_self_component_get_data(
          bt_self_component_sink_as_self_component(self_component_sink));

  const int max_name_size = dispatch->display_name_max_size;

  if (dispatch->display_human) {
    if (dispatch->display_metadata)
      print_metadata(dispatch->metadata);

    if (dispatch->display_compact) {

      print_compact("API calls", dispatch->host2,
                    std::make_tuple("Hostnames", "Processes", "Threads"),
                    max_name_size);

      print_compact("Device profiling", dispatch->device2,
                    std::make_tuple("Hostnames", "Processes", "Threads",
                                    "Device pointers", "Subdevice pointers"),
                    max_name_size);

      print_compact("Explicit memory traffic", dispatch->traffic2,
                    std::make_tuple("Hostnames", "Processes", "Threads"),
                    max_name_size);

    } else {

      print_extended("API calls", dispatch->host2,
                     std::make_tuple("Hostname", "Process", "Thread"),
                     max_name_size);

      print_extended("Device profiling", dispatch->device2,
                     std::make_tuple("Hostname", "Process", "Thread",
                                     "Device pointer", "Subdevice pointer"),
                     max_name_size);

      print_extended("Explicit memory traffic", dispatch->traffic2,
                     std::make_tuple("Hostname", "Process", "Thread"),
                     max_name_size);
    }
  } else {
    nlohmann::json j;
    j["units"] = {{"time", "ns"}, {"size", "bytes"}};
    if (dispatch->display_metadata)
      j["metadata"] = dispatch->metadata;
    if (dispatch->display_compact) {
      if (!dispatch->host2.empty())
        j["host"] = json_compact(dispatch->host2);
      if (!dispatch->device2.empty())
        j["device"] = json_compact(dispatch->device2);
      if (!dispatch->traffic2.empty())
        j["trafic"] = json_compact(dispatch->traffic2);
    } else {
      if (!dispatch->host2.empty())
        j["host"] = json_extented(
            dispatch->host2, std::make_tuple("Hostname", "Process", "Thread"));
      if (!dispatch->device2.empty())
        j["device"] = json_extented(dispatch->device2,
                                    std::make_tuple("Hostname", "Process",
                                                    "Thread", "Device pointer",
                                                    "Subdevice pointer"));
      if (!dispatch->traffic2.empty())
        j["traffic"] =
            json_extented(dispatch->traffic2,
                          std::make_tuple("Hostname", "Process", "Thread"));
    }
    std::cout << j << std::endl;
  }
}

/*
 * Called when the trace processing graph containing the sink component
 * is configured.
 *
 * This is where we can create our upstream message iterator.
 */
bt_component_class_sink_graph_is_configured_method_status
tally_dispatch_graph_is_configured(
    bt_self_component_sink *self_component_sink) {
  /* Retrieve our private data from the component's user data */
  struct tally_dispatch *dispatch =
      (tally_dispatch *)bt_self_component_get_data(
          bt_self_component_sink_as_self_component(self_component_sink));

  /* Borrow our unique port */
  bt_self_component_port_input *in_port =
      bt_self_component_sink_borrow_input_port_by_index(self_component_sink, 0);

  /* Create the uptream message iterator */
  bt_message_iterator_create_from_sink_component(self_component_sink, in_port,
                                                 &dispatch->message_iterator);

  return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}
