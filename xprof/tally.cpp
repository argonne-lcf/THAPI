#include "tally.h"
#include "tally.hpp"
#include "xprof_utils.hpp" //Typedef and hashtuple

auto inline get_common_context_field_host(const bt_event *event) {

  auto dur_tuple0 =
      std::make_tuple(std::make_tuple(0, &bt_field_string_get_value,
                                      (hostname_t) ""), // hostname
                      std::make_tuple(1, &bt_field_integer_signed_get_value,
                                      (process_id_t)0), // process
                      std::make_tuple(2, &bt_field_integer_unsigned_get_value,
                                      (thread_id_t)0),
                      std::make_tuple(4, &bt_field_integer_signed_get_value,
                                      (int)0)); // thread

  const bt_field *common_context_field =
      bt_event_borrow_common_context_field_const(event);

  return thapi_bt2_getter(common_context_field, dur_tuple0);
}

auto inline get_common_context_field(const bt_event *event) {

  auto dur_tuple0 =
      std::make_tuple(std::make_tuple(0, &bt_field_string_get_value,
                                      (hostname_t) ""), // hostname
                      std::make_tuple(1, &bt_field_integer_signed_get_value,
                                      (process_id_t)0), // process
                      std::make_tuple(2, &bt_field_integer_unsigned_get_value,
                                      (thread_id_t)0));

  const bt_field *common_context_field =
      bt_event_borrow_common_context_field_const(event);

  return thapi_bt2_getter(common_context_field, dur_tuple0);
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

      const bt_field *payload_field =
          bt_event_borrow_payload_field_const(event);
      if (strcmp(class_name, "lttng:host") == 0) {
        auto dur_tuple0 = std::make_tuple(
            std::make_tuple(0, bt_field_string_get_value, (std::string) ""),
            std::make_tuple(1, &bt_field_integer_unsigned_get_value,
                            (uint64_t)0),
            std::make_tuple(2, &bt_field_bool_get_value, (bool)0));

        const auto &[hostname, process_id, thread_id, backend_id] =
            get_common_context_field_host(event);
        const auto &[name, dur, err] =
            thapi_bt2_getter(payload_field, dur_tuple0);

        TallyCoreTime a{dur, err};
        dispatch->host[backend_id][hpt_function_name_t(hostname, process_id, thread_id, name)] += a;
      } else if (strcmp(class_name, "lttng:device") == 0) {
        auto dur_tuple0 = std::make_tuple(
            std::make_tuple(0, bt_field_string_get_value, (std::string) ""),
            std::make_tuple(1, &bt_field_integer_unsigned_get_value,
                            (uint64_t)0), // dur
            std::make_tuple(2, &bt_field_integer_unsigned_get_value,
                            (uint64_t)0), // device
            std::make_tuple(3, &bt_field_integer_unsigned_get_value,
                            (uint64_t)0), // subdevice
            std::make_tuple(4, &bt_field_bool_get_value, (bool)0), // Error
            std::make_tuple(5, &bt_field_string_get_value, (std::string) "") ); // Metadata

        const auto &[hostname, process_id, thread_id] =
            get_common_context_field(event);
        const auto &[name, dur, did, sdid, err, metadata] =
            thapi_bt2_getter(payload_field, dur_tuple0);

         /*Should fucking cache this function */
        const auto name_demangled = (dispatch->demangle_name) ? f_demangle_name(name) : name;
        const auto name_with_metadata = (dispatch->display_kernel_verbose && !metadata.empty()) ? name_demangled + "[" + metadata + "]" : name_demangled;

        TallyCoreTime a{dur, err};
        dispatch->device[hpt_device_function_name_t(hostname, process_id, thread_id, did, sdid, name_with_metadata)] += a;

      } else if (strcmp(class_name, "lttng:traffic") == 0) {
        auto dur_tuple0 = std::make_tuple(
            std::make_tuple(0, bt_field_string_get_value, (std::string) ""),
            std::make_tuple(1, &bt_field_integer_unsigned_get_value,
                            (uint64_t)0)); // size

        const auto &[hostname, process_id, thread_id] =
            get_common_context_field(event);
        const auto &[name, size] = thapi_bt2_getter(payload_field, dur_tuple0);
        TallyCoreByte a{(uint64_t)size, false};
        dispatch->traffic[hpt_function_name_t(hostname, process_id, thread_id,
                                               name)] += a;

      } else if (strcmp(class_name, "lttng:device_name") == 0) {
        auto dur_tuple0 = std::make_tuple(
            std::make_tuple(0, &bt_field_string_get_value, (std::string) ""),
            std::make_tuple(1, &bt_field_integer_unsigned_get_value,
                            (thapi_device_id)0)); // device

        const auto &[hostname, process_id, thread_id] = get_common_context_field(event);
        (void) thread_id;
        const auto &[name, did] = thapi_bt2_getter(payload_field, dur_tuple0);

        dispatch->device_name[hp_device_t(hostname, process_id, did)] = name;
      } else if (strcmp(class_name, "lttng_ust_thapi:metadata") == 0) {

        auto dur_tuple0 = std::make_tuple(std::make_tuple(
            0, &bt_field_string_get_value, (std::string) "")); // device

        const auto [metadata] = thapi_bt2_getter(payload_field, dur_tuple0);
        dispatch->metadata.push_back(metadata);
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

  val = bt_value_map_borrow_entry_value_const(params, "display_kernel_verbose");
  const bool display_kernel_verbose =
      (val && bt_value_is_bool(val) ? bt_value_bool_get(val) : false);

  /* Allocate a private data structure */
  struct tally_dispatch *dispatch = new tally_dispatch;

  dispatch->display_compact =
      (display_mode == "compact");                        // Compact or Extented
  dispatch->demangle_name = (display_name == "demangle"); // Demangle or mangle
  dispatch->display_human = (display_human == "human");   // Human or JSON
  dispatch->display_metadata = display_metadata;
  dispatch->display_name_max_size = display_name_max_size;
  dispatch->display_kernel_verbose = display_kernel_verbose;

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

      for (const auto& [level,host]: dispatch->host) {
        (void) level;
        print_compact("API calls", host,
                      std::make_tuple("Hostnames", "Processes", "Threads"),
                      max_name_size);
      }
      print_compact("Device profiling", dispatch->device,
                    std::make_tuple("Hostnames", "Processes", "Threads",
                                    "Devices", "Subdevices"),
                    max_name_size);

      print_compact("Explicit memory traffic", dispatch->traffic,
                    std::make_tuple("Hostnames", "Processes", "Threads"),
                    max_name_size);

    } else {
      for (const auto& [level,host]: dispatch->host) {
        (void) level;
        print_extended("API calls", host,
                       std::make_tuple("Hostname", "Process", "Thread"),
                       max_name_size);
      }
      print_extended("Device profiling", dispatch->device,
                     std::make_tuple("Hostname", "Process", "Thread",
                                     "Device pointer", "Subdevice pointer"),
                     max_name_size);

      print_extended("Explicit memory traffic", dispatch->traffic,
                     std::make_tuple("Hostname", "Process", "Thread"),
                     max_name_size);
    }
  } else {
    nlohmann::json j;
    j["units"] = {{"time", "ns"}, {"size", "bytes"}};
    if (dispatch->display_metadata)
      j["metadata"] = dispatch->metadata;
    if (dispatch->display_compact) {
      if (!dispatch->host[0].empty())
        j["host"] = json_compact(dispatch->host[0]);
      if (!dispatch->device.empty())
        j["device"] = json_compact(dispatch->device);
      if (!dispatch->traffic.empty())
        j["trafic"] = json_compact(dispatch->traffic);
    } else {
      if (!dispatch->host[0].empty())
        j["host"] = json_extented(
            dispatch->host[0], std::make_tuple("Hostname", "Process", "Thread"));
      if (!dispatch->device.empty())
        j["device"] = json_extented(dispatch->device,
                                    std::make_tuple("Hostname", "Process",
                                                    "Thread", "Device pointer",
                                                    "Subdevice pointer"));
      if (!dispatch->traffic.empty())
        j["traffic"] =
            json_extented(dispatch->traffic,
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
