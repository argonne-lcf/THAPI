#ifndef CLINTERVAL_HELPERS_H
#define CLINTERVAL_HELPERS_H

#include "clinterval.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern void init_clinterval_callbacks(struct clinterval_dispatch*);

    bool downstream_message_queue_empty();
    size_t downstream_message_queue_size();
    const bt_message * downstream_message_queue_pop();

#ifdef __cplusplus
}
#endif
#endif
