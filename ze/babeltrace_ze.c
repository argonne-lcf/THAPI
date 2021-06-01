#include "babeltrace_ze.h"

/*
 * Prints a line for `message`, if it's an event message, to the
 * standard output.
 */
//static
//void print_event(struct ze_dispatch *ze_dispatch, struct ze_callbacks *ze_callbacks, const bt_event *event)
//{
//    const bt_event_class *event_class = bt_event_borrow_class_const(event);
//
//    /* Get the number of payload field members */
//    const bt_field *payload_field =
//        bt_event_borrow_payload_field_const(event);
//    uint64_t member_count = bt_field_class_structure_get_member_count(
//        bt_field_borrow_class_const(payload_field));
//
//    /* Write a corresponding line to the standard output */
//    printf("%s (id: %" PRIu64 ") (%" PRIu64 " payload member%s)\n",
//        bt_event_class_get_name(event_class),
//        bt_event_class_get_id(event_class),
//        member_count, member_count == 1 ? "" : "s");
//}

struct ze_event_callbacks * ze_create_event_callbacks(const char *name) {
    intptr_t mem = (intptr_t)calloc(1, sizeof(struct ze_event_callbacks) + strlen(name) + 1);
    struct ze_event_callbacks * callbacks = (struct ze_event_callbacks *)mem;
    callbacks->name = (const char *)(mem + sizeof(struct ze_event_callbacks));
    strcpy((char *)(callbacks->name), name);
    utarray_new(callbacks->callbacks, &ut_ptr_icd);
    return callbacks;
}

void ze_register_dispatcher(struct ze_dispatch *ze_dispatch, const char *name, ze_dispatcher_t *dispatcher) {
    struct ze_event_callbacks *callbacks = NULL;
    HASH_FIND_STR(ze_dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = ze_create_event_callbacks(name);
        HASH_ADD_KEYPTR(hh, ze_dispatch->event_callbacks, callbacks->name, strlen(callbacks->name), callbacks);
    }
    callbacks->dispatcher = dispatcher;
}

void ze_register_callback(struct ze_dispatch *ze_dispatch, const char *name, void *func) {
    struct ze_event_callbacks *callbacks;
    HASH_FIND_STR(ze_dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = ze_create_event_callbacks(name);
        HASH_ADD_STR(ze_dispatch->event_callbacks, name, callbacks);
    }
    if (func)
        utarray_push_back(callbacks->callbacks, &func);
}

/*
 * Initializes the sink component.
 */
static
bt_component_class_initialize_method_status ze_dispatch_initialize(
        bt_self_component_sink *self_component_sink,
        bt_self_component_sink_configuration *configuration,
        const bt_value *params, void *initialize_method_data)
{
    /* Allocate a private data structure */
    struct ze_dispatch *ze_dispatch = calloc(1, sizeof(struct ze_dispatch));

    /* Set the component's user data to our private data structure */
    bt_self_component_set_data(
        bt_self_component_sink_as_self_component(self_component_sink),
        ze_dispatch);

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

    init_ze_dispatchers(ze_dispatch);
    return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the sink component.
 */
static
void ze_dispatch_finalize(bt_self_component_sink *self_component_sink)
{
    /* Retrieve our private data from the component's user data */
    struct ze_dispatch *ze_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    struct ze_callbacks *s, *tmp;
    HASH_ITER(hh, ze_dispatch->callbacks, s, tmp) {
      HASH_DEL(ze_dispatch->callbacks, s);
      free(s);
    }
    struct ze_event_callbacks *s2, *tmp2;
    HASH_ITER(hh, ze_dispatch->event_callbacks, s2, tmp2) {
      HASH_DEL(ze_dispatch->event_callbacks, s2);
      utarray_free(s2->callbacks);
      free(s2);
    }
    /* Free the allocated structure */
    free(ze_dispatch);
}

/*
 * Called when the trace processing graph containing the sink component
 * is configured.
 *
 * This is where we can create our upstream message iterator.
 */
static
bt_component_class_sink_graph_is_configured_method_status
ze_dispatch_graph_is_configured(bt_self_component_sink *self_component_sink)
{
    /* Retrieve our private data from the component's user data */
    struct ze_dispatch *ze_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Borrow our unique port */
    bt_self_component_port_input *in_port =
        bt_self_component_sink_borrow_input_port_by_index(
            self_component_sink, 0);

    /* Create the uptream message iterator */
    bt_message_iterator_create_from_sink_component(self_component_sink,
        in_port, &ze_dispatch->message_iterator);

    return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}

/*
 * Consumes a batch of messages and writes the corresponding lines to
 * the standard output.
 */
static
bt_component_class_sink_consume_method_status ze_dispatch_consume(
        bt_self_component_sink *self_component_sink)
{
    bt_component_class_sink_consume_method_status status =
        BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;

    /* Retrieve our private data from the component's user data */
    struct ze_dispatch *ze_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Consume a batch of messages from the upstream message iterator */
    bt_message_array_const messages;
    uint64_t message_count;
    bt_message_iterator_next_status next_status =
        bt_message_iterator_next(ze_dispatch->message_iterator, &messages,
            &message_count);

    switch (next_status) {
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_END:
        /* End of iteration: put the message iterator's reference */
        bt_message_iterator_put_ref(ze_dispatch->message_iterator);
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
        /* Current message */
        const bt_message *message = messages[i];
        if (bt_message_get_type(message) == BT_MESSAGE_TYPE_EVENT) {
            const bt_event *event = bt_message_event_borrow_event_const(message);
            const bt_event_class *event_class = bt_event_borrow_class_const(event);
            struct ze_callbacks *callbacks = NULL;
            const char * class_name = bt_event_class_get_name(event_class);

            HASH_FIND_STR(ze_dispatch->callbacks, class_name, callbacks);
            if (!callbacks) {
                const size_t class_name_sz = strlen(class_name);
                callbacks = (struct ze_callbacks *)calloc(1, sizeof(struct ze_callbacks) + class_name_sz + 1);
                callbacks->name = (const char *)callbacks + sizeof(struct ze_callbacks);
                strncpy((char *)(callbacks->name), class_name, class_name_sz + 1);
                HASH_ADD_KEYPTR(hh, ze_dispatch->callbacks, class_name, class_name_sz, callbacks);
                struct ze_event_callbacks *event_callbacks = NULL;
                HASH_FIND_STR(ze_dispatch->event_callbacks, class_name, event_callbacks);
                if (event_callbacks) {
                    callbacks->dispatcher = event_callbacks->dispatcher;
                    callbacks->callbacks = event_callbacks->callbacks;
                }
            }
            /* Print line for current message if it's an event message */
            if (callbacks->dispatcher)
                callbacks->dispatcher(ze_dispatch, callbacks, event, bt_message_event_borrow_default_clock_snapshot_const(message));

            /* Put this message's reference */
	}
        bt_message_put_ref(message);
    }

end:
    return status;
}


/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `ze` plugin */
BT_PLUGIN(ze);

/* Define the `text` sink component class */
BT_PLUGIN_SINK_COMPONENT_CLASS(dispatch, ze_dispatch_consume);

/* Set some of the `text` sink component class's optional methods */

BT_PLUGIN_SINK_COMPONENT_CLASS_INITIALIZE_METHOD(dispatch, ze_dispatch_initialize);
BT_PLUGIN_SINK_COMPONENT_CLASS_FINALIZE_METHOD(dispatch, ze_dispatch_finalize);
BT_PLUGIN_SINK_COMPONENT_CLASS_GRAPH_IS_CONFIGURED_METHOD(dispatch, ze_dispatch_graph_is_configured);
