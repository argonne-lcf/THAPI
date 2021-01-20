#include <babeltrace2/babeltrace.h>
#include "clinterval.h"

/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `clprof` plugin */
BT_PLUGIN(clprof);

//~ ~ ~
// Filter Interval
//~ ~ ~

BT_PLUGIN_FILTER_COMPONENT_CLASS(interval, clinterval_dispatch_message_iterator_next);

BT_PLUGIN_FILTER_COMPONENT_CLASS_INITIALIZE_METHOD(interval, clinterval_dispatch_initialize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_FINALIZE_METHOD(interval, clinterval_dispatch_finalize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_MESSAGE_ITERATOR_CLASS_INITIALIZE_METHOD(
    interval, clinterval_dispatch_message_iterator_initialize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_MESSAGE_ITERATOR_CLASS_FINALIZE_METHOD(interval,
    clinterval_dispatch_message_iterator_finalize);

