#include "timeline.h"
#include "timeline.hpp"
#include "xprof_utils.hpp" // typedef

#include <iomanip> // set precision
#include <iostream> // stdcout
bt_component_class_sink_consume_method_status timeline_dispatch_consume(
        bt_self_component_sink *self_component_sink)
{
   bt_component_class_sink_consume_method_status status =
        BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;

    /* Retrieve our private data from the component's user data */
    struct timeline_dispatch *dispatch = (timeline_dispatch*) bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Consume a batch of messages from the upstream message iterator */
    bt_message_array_const messages;
    uint64_t message_count;
    bt_message_iterator_next_status next_status =
        bt_message_iterator_next(dispatch->message_iterator, &messages,
            &message_count);

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
            const char * class_name = bt_event_class_get_name(event_class);

            //Common context field
            const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);

            const bt_field *hostname_field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 0);
            const hostname_t hostname = std::string{bt_field_string_get_value(hostname_field)};

            const bt_field *process_id_field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 1);
            const process_id_t process_id = bt_field_integer_signed_get_value(process_id_field);

            const bt_field *thread_id_field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 2);
            const thread_id_t thread_id = bt_field_integer_unsigned_get_value(thread_id_field);

            const bt_field *ts_field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 3);
            const long ts = bt_field_integer_unsigned_get_value(ts_field);

            //Payload
            const bt_field *payload_field = bt_event_borrow_payload_field_const(event);

            const bt_field *name_field = bt_field_structure_borrow_member_field_by_index_const(payload_field, 0);
            const std::string name = std::string{bt_field_string_get_value(name_field)};

            const bt_field *dur_field = bt_field_structure_borrow_member_field_by_index_const(payload_field, 1);
            const long dur = bt_field_integer_unsigned_get_value(dur_field);

            if (std::string(class_name) == "lttng:host") {
                std::cout << "{\"pid\": " <<  "\"" + hostname << "-" << process_id  <<   "\""
                          << ",\"tid\":" <<  thread_id 
                          << std::fixed << std::setprecision(2) << ",\"ts\":" << ts*1.E-3 
                          << ",\"dur\":" << dur*1.E-3
                          << ",\"name\":" << "\"" << name << "\"" 
                          << ",\"ph\":\"X\""
                          << "}," << std::endl;
            } else if ( std::string(class_name) == "lttng:device" )  {
               const bt_field *did_field = bt_field_structure_borrow_member_field_by_index_const(payload_field, 2);
               const thapi_device_id did = bt_field_integer_unsigned_get_value(did_field);
 
               std::cout << "{\"pid\": " <<  "\"" + hostname << "-" << process_id  <<   "\""
                         << ",\"tid\":" <<  "\"GPU-"<< did << "\""
                         << std::fixed << std::setprecision(2) << ",\"ts\":" << ts*1.E-3
                         << ",\"dur\":" << dur*1.E-3
                         << ",\"name\":" << "\"" << name << "\""
                         << ",\"ph\":\"X\""
                         << "}," << std::endl;
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
bt_component_class_initialize_method_status timeline_dispatch_initialize(
        bt_self_component_sink *self_component_sink,
        bt_self_component_sink_configuration *configuration,
        const bt_value *params, void *initialize_method_data)
{
    /* Allocate a private data structure */
    struct timeline_dispatch *dispatch = new timeline_dispatch;  

    /* Set the component's user data to our private data structure */
    bt_self_component_set_data(
        bt_self_component_sink_as_self_component(self_component_sink),
        dispatch);

    /*
     * Add an input port named `in` to the sink component.
     *
     * This is needed so that this sink component can be connected to a
     * filter or a source component. With a connected upstream
     * component, this sink component can create a message iterator
     * to consume messages.
     */
    bt_self_component_sink_add_input_port(self_component_sink,
        "in", NULL, NULL);
   
    /* Print the begining of the json */
    std::cout << "{\"traceEvents\":[" << std::endl; 

    return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the sink component.
 */
void timeline_dispatch_finalize(bt_self_component_sink *self_component_sink)
{
    
    // JSTON is stupornd, it doesn't allow trailling comma. 
    // To avoid creating yet anothing state, we will create a dummy/empty object at the end. 
    // This only work because chrome::tracing doesn't parse such event.
    std::cout << "{}]}" << std::endl;

}

/*
 * Called when the trace processing graph containing the sink component
 * is configured.
 *
 * This is where we can create our upstream message iterator.
 */
bt_component_class_sink_graph_is_configured_method_status
timeline_dispatch_graph_is_configured(bt_self_component_sink *self_component_sink)
{
    /* Retrieve our private data from the component's user data */
    struct timeline_dispatch *dispatch = (timeline_dispatch*) bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Borrow our unique port */
    bt_self_component_port_input *in_port =
        bt_self_component_sink_borrow_input_port_by_index(
            self_component_sink, 0);

    /* Create the uptream message iterator */
    bt_message_iterator_create_from_sink_component(self_component_sink,
        in_port, &dispatch->message_iterator);

    return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}


