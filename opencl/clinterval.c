#include "clinterval.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <babeltrace2/babeltrace.h>
#include "uthash.h"
#include "utarray.h"


// Global variable used to path some handle from the 
// clinterval_dispatch_message_iterator_next to the callbacks
struct clinterval_message_iterator *clinterval_iter_g = NULL;
bt_self_message_iterator *self_message_iterator_g = NULL;

static
struct clinterval_event_callbacks * opencl_create_event_callbacks(const char *name) {
    intptr_t mem = (intptr_t)calloc(1, sizeof(struct clinterval_event_callbacks) + strlen(name) + 1);
    struct clinterval_event_callbacks * callbacks = (struct clinterval_event_callbacks *)mem;
    callbacks->name = (const char *)(mem + sizeof(struct clinterval_event_callbacks));
    strcpy((char *)(callbacks->name), name);
    utarray_new(callbacks->callbacks, &ut_ptr_icd);
    return callbacks;
}

// `clinterval_dispatch -> clinterval_eventcallbacks -> clinterval_dispatcher`
void clinterval_register_dispatcher(struct clinterval_dispatch *dispatch, const char *name, clinterval_dispatcher_t *dispatcher) {
    struct clinterval_event_callbacks *callbacks = NULL;
    HASH_FIND_STR(dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = opencl_create_event_callbacks(name);
        HASH_ADD_KEYPTR(hh, dispatch->event_callbacks, callbacks->name, strlen(callbacks->name), callbacks);
    }
    callbacks->dispatcher = dispatcher;
}

// `clinterval_dispatch -> clinterval_eventcallbacks`
void clinterval_register_callback(struct clinterval_dispatch *dispatch, const char *name, void *func) {
    struct clinterval_event_callbacks *callbacks;
    HASH_FIND_STR(dispatch->event_callbacks, name, callbacks);
    if (!callbacks) {
        callbacks = opencl_create_event_callbacks(name);
        HASH_ADD_STR(dispatch->event_callbacks, name, callbacks);
    }
    if (func){
        utarray_push_back(callbacks->callbacks, &func);
   }
}


/*
 * Create downstream message event class
 */ 
bt_event_class* create_host_event_class_message(bt_trace_class *trace_class, bt_stream_class *stream_class)
{
    bt_event_class *event_class = bt_event_class_create(stream_class);
    bt_event_class_set_name(event_class, "lttng:host");

    /* Common field */
    bt_field_class *context_field_class = bt_field_class_structure_create(trace_class);

    // Host Name
    bt_field_class *hostname_field_class = bt_field_class_string_create(trace_class);
    bt_field_class_structure_append_member(context_field_class, "hostname", hostname_field_class);

    // vpid
    bt_field_class *vpid_context_class= bt_field_class_integer_signed_create(trace_class);
    bt_field_class_structure_append_member(context_field_class, "vpid", vpid_context_class);

    // vtid
    bt_field_class *pthread_id_context_class= bt_field_class_integer_signed_create(trace_class);
    bt_field_class_structure_append_member(context_field_class, "vtid", pthread_id_context_class);

    // ts
    bt_field_class *ts_context_class= bt_field_class_integer_unsigned_create(trace_class);
    bt_field_class_structure_append_member(context_field_class, "ts", ts_context_class);

    // Put the ref and clean
    bt_stream_class_set_event_common_context_field_class(stream_class, context_field_class);
    bt_field_class_put_ref(vpid_context_class);
    bt_field_class_put_ref(pthread_id_context_class);
    bt_field_class_put_ref(context_field_class);


    /* Payload */
    bt_field_class *payload_field_class = bt_field_class_structure_create(trace_class);

    // Name
    bt_field_class *name_field_class = bt_field_class_string_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "name", name_field_class);

    // Duration */
    bt_field_class *dur_field_class = bt_field_class_integer_unsigned_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "dur", dur_field_class);

    // Error code */
    bt_field_class *err_field_class = bt_field_class_bool_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "err", err_field_class);

    // Put the ref and clean
    bt_event_class_set_payload_field_class(event_class, payload_field_class);
    bt_field_class_put_ref(payload_field_class);
    bt_field_class_put_ref(hostname_field_class);
    bt_field_class_put_ref(name_field_class);
    bt_field_class_put_ref(dur_field_class);
    bt_field_class_put_ref(err_field_class);

    return event_class;
}

bt_event_class* create_device_event_class_message(bt_trace_class *trace_class, bt_stream_class *stream_class)
{
    bt_event_class *event_class = bt_event_class_create(stream_class);
    bt_event_class_set_name(event_class, "lttng:device");

    // No need to set the common field,
    // Already set by `create_host_event_class_message`
    
    /* Payload */
    bt_field_class *payload_field_class = bt_field_class_structure_create(trace_class);

    // Name
    bt_field_class *name_field_class = bt_field_class_string_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "name", name_field_class);

    // Duration
    bt_field_class *dur_field_class = bt_field_class_integer_unsigned_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "dur", dur_field_class);

    // Would like to put `did` and `sdid` in the `specific context field`, but I don't know how to them afterward in `create_and_enqueue_device_message`

    // did (device)
    bt_field_class *device_id_field_class= bt_field_class_integer_unsigned_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "did", device_id_field_class);

    // sdid (subdevice) 
    bt_field_class *subdevice_id_field_class= bt_field_class_integer_unsigned_create(trace_class);
    bt_field_class_structure_append_member(payload_field_class, "sdid", subdevice_id_field_class);


    // Put the ref and clean
    bt_event_class_set_payload_field_class(event_class, payload_field_class);
    bt_field_class_put_ref(payload_field_class);
    bt_field_class_put_ref(name_field_class);
    bt_field_class_put_ref(dur_field_class);
    bt_field_class_put_ref(device_id_field_class);
    bt_field_class_put_ref(subdevice_id_field_class);

    return event_class;
}
/*
 * Initializes the filter component.
 */
bt_component_class_initialize_method_status clinterval_dispatch_initialize(
        bt_self_component_filter *self_component_filter,
        bt_self_component_filter_configuration *configuration,
        const bt_value *params, void *initialize_method_data)
{
    /* Allocate a private data structure */
    struct clinterval_dispatch *dispatch = calloc(1, sizeof(struct clinterval_dispatch));

    /* Set the component's user data to our private data structure */
    bt_self_component_set_data(
        bt_self_component_filter_as_self_component(self_component_filter),
        dispatch);

    /*
     * Add an input port named `in` to the filter component.
     *
     * This is needed so that this filter component can be connected to
     * a filter or a source component. With a connected upstream
     * component, this filter component's message iterator can create a
     * message iterator to consume messages.
     *
     * Add an output port named `out` to the filter component.
     *
     * This is needed so that this filter component can be connected to
     * a filter or a sink component. Once a downstream component is
     * connected, it can create our message iterator.
     */
    bt_self_component_filter_add_input_port(self_component_filter,
        "in", NULL, &dispatch->in_port);
    bt_self_component_filter_add_output_port(self_component_filter,
        "out", NULL, NULL);

    /* Create message that will be used by the filter */
    bt_self_component *self_component = bt_self_component_filter_as_self_component(self_component_filter);
    /* Create a default trace class */
    bt_trace_class *trace_class = bt_trace_class_create(self_component);

    /* Create a stream trace class within `trace_class` */
    bt_stream_class *stream_class = bt_stream_class_create(trace_class);

    // We don't create a clock, because we ensure monotonic order for the downstream messages. 
    dispatch->host_event_class = create_host_event_class_message(trace_class,stream_class);
    dispatch->device_event_class = create_device_event_class_message(trace_class, stream_class);

    /* Create a default trace from (instance of `trace_class`) */
    bt_trace *trace = bt_trace_create(trace_class);

    /*
     * Create the source component's stream (instance of `stream_class`
     * within `trace`).
     */
    dispatch->stream = bt_stream_create(stream_class, trace);

    /* Put the references we don't need anymore */
    bt_trace_put_ref(trace);
    bt_stream_class_put_ref(stream_class);
    bt_trace_class_put_ref(trace_class);

    /* Will call clinterval_register_dispatcher */
    /* For each dispacther automaticaly generated */
    init_clinterval_dispatch(dispatch);

    /*
    clinterval_dispatch ->  clinterval_callbacks
    clinterval_callbacks -> {clinterval_dispatcher, UT_array callbacks}
    */

    /* Will call `clinterval_register_callback` for each callback functions implemented in `clinterval_callbacks` */
    /* Hence will populate clinterval_dispatch ->  clinterval_eventcallbacks -> UT_array callbacks */
    init_clinterval_callbacks(dispatch);
    return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the filter component.
 */
void clinterval_dispatch_finalize(bt_self_component_filter *self_component_filter)
{
    /* Retrieve our private data from the component's user data */
    struct clinterval_dispatch *clinterval_dispatch = bt_self_component_get_data(
        bt_self_component_filter_as_self_component(self_component_filter));

    struct clinterval_callbacks *s, *tmp;
    HASH_ITER(hh, clinterval_dispatch->callbacks, s, tmp) {
      HASH_DEL(clinterval_dispatch->callbacks, s);
      free(s);
    }
    struct clinterval_event_callbacks *s2, *tmp2;
    HASH_ITER(hh, clinterval_dispatch->event_callbacks, s2, tmp2) {
      HASH_DEL(clinterval_dispatch->event_callbacks, s2);
      utarray_free(s2->callbacks);
      free(s2);
    }
    /* Free the allocated structure */
    free(clinterval_dispatch);
}

/*
 * Initializes the message iterator.
 */
bt_message_iterator_class_initialize_method_status
clinterval_dispatch_message_iterator_initialize(
        bt_self_message_iterator *self_message_iterator,
        bt_self_message_iterator_configuration *configuration,
        bt_self_component_port_output *self_port)
{

    /* Allocate a private data structure */
    struct clinterval_message_iterator *dispatch_iter =
        malloc(sizeof(struct clinterval_message_iterator));

    /* Retrieve the component's private data from its user data */
    struct clinterval_dispatch *clinterval_dispatch = bt_self_component_get_data(
        bt_self_message_iterator_borrow_component(self_message_iterator));

    /* Keep a link to the component's private data */
    dispatch_iter->dispatch = clinterval_dispatch;

    /* Initliaze the value */
    dispatch_iter->callbacks_state = init_clinterval_callbacks_state();

    /* Create the uptream message iterator */
    bt_message_iterator_create_from_message_iterator(self_message_iterator,
        clinterval_dispatch->in_port, &dispatch_iter->message_iterator);

    /* Set the message iterator's user data to our private data structure */
    bt_self_message_iterator_set_data(self_message_iterator, dispatch_iter);
    return BT_MESSAGE_ITERATOR_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

/*
 * Finalizes the message iterator.
 */
void clinterval_dispatch_message_iterator_finalize(
        bt_self_message_iterator *self_message_iterator)
{
    /* Retrieve our private data from the message iterator's user data */
    struct clinterval_dispatch_message_iterator *dispatch_iter =
        bt_self_message_iterator_get_data(self_message_iterator);

    /* Free the allocated structure */
    free(dispatch_iter);
}

/*
 * Returns the next message to the message iterator's user.
 *
 * This method can fill the `messages` array with up to `capacity`
 * messages.
 * 
 * We handle the case, where we generate more message than we consume.
 */
bt_message_iterator_class_next_method_status clinterval_dispatch_message_iterator_next(
        bt_self_message_iterator *self_message_iterator,
        bt_message_array_const messages, uint64_t capacity,
        uint64_t *count)
{

   /* Retrieve our private data from the message iterator's user data */
    struct clinterval_message_iterator *dispatch_iter =
        bt_self_message_iterator_get_data(self_message_iterator);

    /* Consume a batch of messages from the upstream message iterator */
    bt_message_array_const upstream_messages;
    uint64_t upstream_message_count;
    bt_message_iterator_next_status next_status;

    /* Set global variable */
    clinterval_iter_g = dispatch_iter;
    self_message_iterator_g = self_message_iterator;

consume_upstream_messages:

    next_status = bt_message_iterator_next(dispatch_iter->message_iterator,
        &upstream_messages, &upstream_message_count);

    /* Initialize the return status to a success */
    bt_message_iterator_class_next_method_status status =
        BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_OK;
     
    if ( (next_status == BT_MESSAGE_ITERATOR_NEXT_STATUS_END) && downstream_message_queue_empty(dispatch_iter) ) {
       bt_message_iterator_put_ref(dispatch_iter->message_iterator);
       status = BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_END;
       goto end;
    }
 
    switch (next_status) {
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_AGAIN:
        status = BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_AGAIN;
        goto end;
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_MEMORY_ERROR:
        status = BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_MEMORY_ERROR;
        goto end;
    case BT_MESSAGE_ITERATOR_NEXT_STATUS_ERROR:
        status = BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_ERROR;
        goto end;
    default:
        break;
    }

    /* For each consumed message */
    for (uint64_t upstream_i = 0; upstream_i < upstream_message_count;
            upstream_i++) {
        /* Current message */
        const bt_message *upstream_message = upstream_messages[upstream_i];

        if (bt_message_get_type(upstream_message) == BT_MESSAGE_TYPE_EVENT) {
            const bt_event *event = bt_message_event_borrow_event_const(upstream_message);
            const bt_event_class *event_class = bt_event_borrow_class_const(event);
            struct clinterval_callbacks *callbacks = NULL;
            const char * class_name = bt_event_class_get_name(event_class);
            
            HASH_FIND_STR(dispatch_iter->dispatch->callbacks, class_name, callbacks);
            if (!callbacks) {
                const size_t class_name_sz = strlen(class_name);
                callbacks = (struct clinterval_callbacks *)calloc(1, sizeof(struct clinterval_callbacks) + class_name_sz + 1);
                callbacks->name = (const char *)callbacks + class_name_sz;
                strncpy((char *)(callbacks->name), class_name, class_name_sz + 1);
                HASH_ADD_KEYPTR(hh, dispatch_iter->dispatch->callbacks, class_name, class_name_sz, callbacks);
                struct clinterval_event_callbacks *event_callbacks = NULL;
                HASH_FIND_STR(dispatch_iter->dispatch->event_callbacks, class_name, event_callbacks);
                if (event_callbacks) {
                    callbacks->dispatcher = event_callbacks->dispatcher;
                    callbacks->callbacks = event_callbacks->callbacks;
                }
            }
            if (callbacks->dispatcher) {
                // Will add message to `downstream_message_queue` global variable.
                callbacks->dispatcher(dispatch_iter->dispatch, callbacks, event, bt_message_event_borrow_default_clock_snapshot_const(upstream_message));
            }
        }
        /* Discard upstream message: put its reference */
       bt_message_put_ref(upstream_message);
    }
    if ( downstream_message_queue_empty(dispatch_iter) ) {
        /*
         * We discarded all the upstream messages: get a new batch of
         * messages, because this method _cannot_ return
         * `BT_MESSAGE_ITERATOR_CLASS_NEXT_METHOD_STATUS_OK` and put no
         * messages into its output message array.
         */
        goto consume_upstream_messages;
    }

   /*
    * Pop the maximun number of message allowed to be sended downstream
    */
    const uint64_t N = downstream_message_queue_size(dispatch_iter);
    const uint64_t N_message_to_send = capacity < N ? capacity : N;
    for (uint64_t i = 0; i < N_message_to_send; i++) {
        messages[i] = downstream_message_queue_pop(dispatch_iter);
    }
    *count = N_message_to_send;
end:
    return status;
}

