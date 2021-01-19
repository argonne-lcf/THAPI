#ifndef _CLINTERVAL_H
#define _CLINTERVAL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <babeltrace2/babeltrace.h>
#include "uthash.h"
#include "utarray.h"

#ifdef __cplusplus
extern "C" {
#endif

struct clinterval_dispatch;
struct clinterval_callbacks;

extern void init_clinterval_dispatch(struct clinterval_dispatch *dispatch);

typedef void (clinterval_dispatcher_t)
    (struct clinterval_dispatch   *dispatch,
     struct clinterval_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct clinterval_callbacks {
    const char *name;
    clinterval_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct clinterval_event_callbacks {
    const char *name;
    clinterval_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct clinterval_dispatch {
    /* Hash table */
    struct clinterval_callbacks *callbacks;
    /* Hash table by name */
    struct clinterval_event_callbacks *event_callbacks;

    bt_stream *stream;
    bt_event_class *host_event_class;
    bt_event_class *device_event_class;

    /* Component's input port (weak) */
    bt_self_component_port_input *in_port;
};

/* Message iterator's private data */
struct clinterval_message_iterator {
    /* (Weak) link to the component's private data */
    struct clinterval_dispatch *dispatch;

    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;
    /* All the variable need to handle the create of the message */
    /* It's a C++ struct */
    void * callbacks_state;
};


extern
bt_component_class_initialize_method_status clinterval_dispatch_initialize(
        bt_self_component_filter *self_component_filter,
        bt_self_component_filter_configuration *configuration,
        const bt_value *params, void *initialize_method_data);

extern
void clinterval_dispatch_finalize(bt_self_component_filter *self_component_filter);

extern void
clinterval_register_dispatcher(struct clinterval_dispatch *dispatch,
                           const char *name,
                           clinterval_dispatcher_t *dispatcher);

extern void
clinterval_register_callback(struct clinterval_dispatch *dispatch,
                         const char *name,
                         void *func);

extern
bt_message_iterator_class_next_method_status clinterval_dispatch_message_iterator_next(
        bt_self_message_iterator *self_message_iterator,
        bt_message_array_const messages, uint64_t capacity,
        uint64_t *count);


extern
bt_message_iterator_class_initialize_method_status
clinterval_dispatch_message_iterator_initialize(
        bt_self_message_iterator *self_message_iterator,
        bt_self_message_iterator_configuration *configuration,
        bt_self_component_port_output *self_port);

extern
void clinterval_dispatch_message_iterator_finalize(
        bt_self_message_iterator *self_message_iterator);

// Global state for the downstream message
extern struct clinterval_message_iterator *clinterval_iter_g;
extern bt_self_message_iterator *self_message_iterator_g;

#ifdef __cplusplus
}
#endif

#endif
