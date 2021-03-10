#include <babeltrace2/babeltrace.h>
#include "zeinterval.h"

/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `zeprof` plugin */
BT_PLUGIN(zeprof);

//~ ~ ~
// Filter Interval
//~ ~ ~

BT_PLUGIN_FILTER_COMPONENT_CLASS(interval, zeinterval_dispatch_message_iterator_next);

BT_PLUGIN_FILTER_COMPONENT_CLASS_INITIALIZE_METHOD(interval, zeinterval_dispatch_initialize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_FINALIZE_METHOD(interval, zeinterval_dispatch_finalize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_MESSAGE_ITERATOR_CLASS_INITIALIZE_METHOD(
    interval, zeinterval_dispatch_message_iterator_initialize);
BT_PLUGIN_FILTER_COMPONENT_CLASS_MESSAGE_ITERATOR_CLASS_FINALIZE_METHOD(interval,
    zeinterval_dispatch_message_iterator_finalize);

