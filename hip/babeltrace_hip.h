#ifndef _BABELTRACE_HIP_H
#define _BABELTRACE_HIP_H

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

struct hip_dispatch;
struct hip_callbacks;

extern void init_hip_dispatchers(struct hip_dispatch *hip_dispatch);

typedef void (hip_dispatcher_t)
    (struct hip_dispatch   *hip_dispatch,
     struct hip_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct hip_callbacks {
    const char* name;
    hip_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct hip_event_callbacks {
    const char *name;
    hip_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct hip_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct hip_callbacks *callbacks;
    /* Hash table by name */
    struct hip_event_callbacks *event_callbacks;
};


extern void
hip_register_dispatcher(struct hip_dispatch *hip_dispatch,
                         const char *name,
                         hip_dispatcher_t *dispatcher);

extern void
hip_register_callback(struct hip_dispatch *hip_dispatch,
                     const char *name,
                     void *func);
#ifdef __cplusplus
}
#endif

#endif
