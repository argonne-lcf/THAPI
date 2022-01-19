#ifndef _BABELTRACE_CUDA_H
#define _BABELTRACE_CUDA_H

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

struct cuda_dispatch;
struct cuda_callbacks;

extern void init_cuda_dispatchers(struct cuda_dispatch *cuda_dispatch);

typedef void (cuda_dispatcher_t)
    (struct cuda_dispatch   *cuda_dispatch,
     struct cuda_callbacks *callbacks,
     const bt_event          *message,
     const bt_clock_snapshot *clock);

struct cuda_callbacks {
    const char* name;
    cuda_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

struct cuda_event_callbacks {
    const char *name;
    cuda_dispatcher_t *dispatcher;
    UT_array *callbacks;
    UT_hash_handle hh;
};

/* Sink component's private data */
struct cuda_dispatch {
    /* Upstream message iterator (owned by this) */
    bt_message_iterator *message_iterator;

    /* Hash table */
    struct cuda_callbacks *callbacks;
    /* Hash table by name */
    struct cuda_event_callbacks *event_callbacks;
};


extern void
cuda_register_dispatcher(struct cuda_dispatch *cuda_dispatch,
                         const char *name,
                         cuda_dispatcher_t *dispatcher);

extern void
cuda_register_callback(struct cuda_dispatch *cuda_dispatch,
                     const char *name,
                     void *func);
#ifdef __cplusplus
}
#endif

#endif
