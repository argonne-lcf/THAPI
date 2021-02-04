#ifndef _BABELTRACE_ZE_H
#define _BABELTRACE_ZE_H

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

struct babeltrace_ze_dispatch;
struct babeltrace_ze_callbacks;

extern void init_babeltrace_ze_dispatch(struct babeltrace_ze_dispatch *ze_dispatch);

typedef void (babeltrace_ze_dispatcher_t)
    (struct babeltrace_ze_dispatch   *ze_dispatch,
     struct babeltrace_ze_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct babeltrace_ze_callbacks {
    const char* name;
    babeltrace_ze_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct babeltrace_ze_event_callbacks {
    const char *name;
    babeltrace_ze_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct babeltrace_ze_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct babeltrace_ze_callbacks *callbacks;
    /* Hash table by name */
    struct babeltrace_ze_event_callbacks *event_callbacks;
};


extern void
babeltrace_ze_register_dispatcher(struct babeltrace_ze_dispatch *ze_dispatch,
                       const char *name,
                       babeltrace_ze_dispatcher_t *dispatcher);

extern void
babeltrace_ze_register_callback(struct babeltrace_ze_dispatch *ze_dispatch,
                     const char *name,
                     void *func);
#ifdef __cplusplus
}
#endif

#endif
