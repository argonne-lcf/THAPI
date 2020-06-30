#ifndef _BABELTRACE_CL_H
#define _BABELTRACE_CL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <babeltrace2/babeltrace.h>
#include "uthash.h"
#include "utarray.h"

struct opencl_dispatch;
struct opencl_callbacks;

extern void init_dispatchers(struct opencl_dispatch *opencl_dispatch);

typedef void (opencl_dispatcher_t)
    (struct opencl_dispatch   *opencl_dispatch,
     struct opencl_callbacks *callbacks,
     const bt_event          *message);

struct opencl_unique_id {
    uint64_t class_id;
    uint64_t stream_id;
};


struct opencl_callbacks {
    struct opencl_unique_id id;
    opencl_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct opencl_event_callbacks {
    const char *name;
    opencl_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct opencl_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct opencl_callbacks *callbacks;
    /* Hash table by name */
    struct opencl_event_callbacks *event_callbacks;
};


extern void
opencl_register_dispatcher(struct opencl_dispatch *opencl_dispatch,
                           const char *name,
                           opencl_dispatcher_t *dispatcher);
#endif
