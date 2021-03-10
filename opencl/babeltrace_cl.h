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

#ifdef __cplusplus
extern "C" {
#endif

struct babeltrace_cl_dispatch;
struct babeltrace_cl_callbacks;

extern void init_babeltrace_cl_dispatcher(struct babeltrace_cl_dispatch *dispatch);

typedef void (babeltrace_cl_dispatcher_t)
    (struct babeltrace_cl_dispatch   *dispatch,
     struct babeltrace_cl_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct babeltrace_cl_callbacks {
    const char *name;
    babeltrace_cl_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct babeltrace_cl_event_callbacks {
    const char *name;
    babeltrace_cl_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct babeltrace_cl_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct babeltrace_cl_callbacks *callbacks;
    /* Hash table by name */
    struct babeltrace_cl_event_callbacks *event_callbacks;
};


extern void
babeltrace_cl_register_dispatcher(struct babeltrace_cl_dispatch *dispatch,
                           const char *name,
                           babeltrace_cl_dispatcher_t *dispatcher);

extern void
babeltrace_cl_register_callback(struct babeltrace_cl_dispatch *dispatch,
                         const char *name,
                         void *func);
#ifdef __cplusplus
}
#endif

#endif
