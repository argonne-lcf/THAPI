#ifndef CLINTERVAL_HELPERS_H
#define CLINTERVAL_HELPERS_H

#include "clinterval.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern void init_clinterval_callbacks(struct clinterval_dispatch*);

    extern void* init_clinterval_callbacks_state();

    bool downstream_message_queue_empty(struct clinterval_message_iterator*);
    size_t downstream_message_queue_size(struct clinterval_message_iterator*);
    const bt_message * downstream_message_queue_pop(struct clinterval_message_iterator*);

#ifdef __cplusplus
}
#endif
#endif
