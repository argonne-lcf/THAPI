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

struct ze_dispatch;
struct ze_callbacks;

extern void init_ze_dispatchers(struct ze_dispatch *ze_dispatch);

typedef void (ze_dispatcher_t)
    (struct ze_dispatch   *ze_dispatch,
     struct ze_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct ze_callbacks {
    const char* name;
    ze_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct ze_event_callbacks {
    const char *name;
    ze_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct ze_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct ze_callbacks *callbacks;
    /* Hash table by name */
    struct ze_event_callbacks *event_callbacks;
};


extern void
ze_register_dispatcher(struct ze_dispatch *ze_dispatch,
                       const char *name,
                       ze_dispatcher_t *dispatcher);

extern void
ze_register_callback(struct ze_dispatch *ze_dispatch,
                     const char *name,
                     void *func);
#ifdef __cplusplus
}
#endif

#endif
