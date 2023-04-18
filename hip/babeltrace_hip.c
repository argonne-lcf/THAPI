#include "babeltrace_hip.h"

struct hip_event_callbacks * hip_create_event_callbacks(const char *name) {
    intptr_t mem = (intptr_t)calloc(1, sizeof(struct hip_event_callbacks) + strlen(name) + 1);
    struct hip_event_callbacks * callbacks = (struct hip_event_callbacks *)mem;
    callbacks->name = (const char *)(mem + sizeof(struct hip_event_callbacks));
    strcpy((char *)(callbacks->name), name);
    utarray_new(callbacks->callbacks, &ut_ptr_icd);
    return callbacks;
}

void hip_register_dispatcher(struct hip_dispatch *hip_dispatch, const char *name, hip_dispatcher_t *dispatcher) {
    struct hip_event_callbacks *callbacks = NULL;
    HASH_FIND_STR(hip_dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = hip_create_event_callbacks(name);
        HASH_ADD_KEYPTR(hh, hip_dispatch->event_callbacks, callbacks->name, strlen(callbacks->name), callbacks);
    }
    callbacks->dispatcher = dispatcher;
}

void hip_register_callback(struct hip_dispatch *hip_dispatch, const char *name, void *func) {
    struct hip_event_callbacks *callbacks = NULL ;
    HASH_FIND_STR(hip_dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = hip_create_event_callbacks(name);
        HASH_ADD_STR(hip_dispatch->event_callbacks, name, callbacks);
    }
    if (func)
        utarray_push_back(callbacks->callbacks, &func);
}

/*
 * Initializes the sink component.
 */
static
bt_component_class_initialize_method_status hip_dispatch_initialize(
        bt_self_component_sink *self_component_sink,
        bt_self_component_sink_configuration *configuration,
        const bt_value *params, void *initialize_method_data)
{
    /* Allocate a private data structure */
    struct hip_dispatch *hip_dispatch = calloc(1, sizeof(struct hip_dispatch));

    /* Set the component's user data to our private data structure */
    bt_self_component_set_data(
        bt_self_component_sink_as_self_component(self_component_sink),
        hip_dispatch);

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

    init_hip_dispatchers(hip_dispatch);
    return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the sink component.
 */
static
void hip_dispatch_finalize(bt_self_component_sink *self_component_sink)
{
    /* Retrieve our private data from the component's user data */
    struct hip_dispatch *hip_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    struct hip_callbacks *s, *tmp;
    HASH_ITER(hh, hip_dispatch->callbacks, s, tmp) {
      HASH_DEL(hip_dispatch->callbacks, s);
      free(s);
    }
    struct hip_event_callbacks *s2, *tmp2;
    HASH_ITER(hh, hip_dispatch->event_callbacks, s2, tmp2) {
      HASH_DEL(hip_dispatch->event_callbacks, s2);
      utarray_free(s2->callbacks);
      free(s2);
    }
    /* Free the allocated structure */
    free(hip_dispatch);
}

/*
 * Called when the trace processing graph containing the sink component
 * is configured.
 *
 * This is where we can create our upstream message iterator.
 */
static
bt_component_class_sink_graph_is_configured_method_status
hip_dispatch_graph_is_configured(bt_self_component_sink *self_component_sink)
{
    /* Retrieve our private data from the component's user data */
    struct hip_dispatch *hip_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Borrow our unique port */
    bt_self_component_port_input *in_port =
        bt_self_component_sink_borrow_input_port_by_index(
            self_component_sink, 0);

    /* Create the uptream message iterator */
    bt_message_iterator_create_from_sink_component(self_component_sink,
        in_port, &hip_dispatch->message_iterator);

    return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}

/*
 * Consumes a batch of messages and writes the corresponding lines to
 * the standard output.
 */
static
bt_component_class_sink_consume_method_status hip_dispatch_consume(
        bt_self_component_sink *self_component_sink)
{
    bt_component_class_sink_consume_method_status status =
        BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;

    /* Retrieve our private data from the component's user data */
    struct hip_dispatch *hip_dispatch = bt_self_component_get_data(
        bt_self_component_sink_as_self_component(self_component_sink));

    /* Consume a batch of messages from the upstream message iterator */
    bt_message_array_const messages;
    uint64_t message_count;
    bt_message_iterator_next_status next_status =
        bt_message_iterator_next(hip_dispatch->message_iterator, &messages,
            &message_count);

    switch (next_status) {
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_END:
        /* End of iteration: put the message iterator's reference */
        bt_message_iterator_put_ref(hip_dispatch->message_iterator);
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
            struct hip_callbacks *callbacks = NULL;
            const char * class_name = bt_event_class_get_name(event_class);

            HASH_FIND_STR(hip_dispatch->callbacks, class_name, callbacks);
            if (!callbacks) {
                const size_t class_name_sz = strlen(class_name);
                callbacks = (struct hip_callbacks *)calloc(1, sizeof(struct hip_callbacks) + class_name_sz + 1);
                callbacks->name = (const char *)callbacks + sizeof(struct hip_callbacks);
                strncpy((char *)(callbacks->name), class_name, class_name_sz + 1);
                HASH_ADD_KEYPTR(hh, hip_dispatch->callbacks, class_name, class_name_sz, callbacks);
                struct hip_event_callbacks *event_callbacks = NULL;
                HASH_FIND_STR(hip_dispatch->event_callbacks, class_name, event_callbacks);
                if (event_callbacks) {
                    callbacks->dispatcher = event_callbacks->dispatcher;
                    callbacks->callbacks = event_callbacks->callbacks;
                }
            }
            /* Print line for current message if it's an event message */
            if (callbacks->dispatcher)
                callbacks->dispatcher(hip_dispatch, callbacks, event, bt_message_event_borrow_default_clock_snapshot_const(message));

            /* Put this message's reference */
	}
        bt_message_put_ref(message);
    }

end:
    return status;
}


/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `hip` plugin */
BT_PLUGIN(hip);

/* Define the `text` sink component class */
BT_PLUGIN_SINK_COMPONENT_CLASS(dispatch, hip_dispatch_consume);

/* Set some of the `text` sink component class's optional methods */

BT_PLUGIN_SINK_COMPONENT_CLASS_INITIALIZE_METHOD(dispatch, hip_dispatch_initialize);
BT_PLUGIN_SINK_COMPONENT_CLASS_FINALIZE_METHOD(dispatch, hip_dispatch_finalize);
BT_PLUGIN_SINK_COMPONENT_CLASS_GRAPH_IS_CONFIGURED_METHOD(dispatch, hip_dispatch_graph_is_configured);
