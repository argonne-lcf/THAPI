#include <babeltrace2/babeltrace.h>
#include "tally.h"
/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `xprof` plugin */
BT_PLUGIN(xprof);

//~ ~ ~
// Sink Tally
//~ ~ ~

BT_PLUGIN_SINK_COMPONENT_CLASS(tally, tally_dispatch_consume);

BT_PLUGIN_SINK_COMPONENT_CLASS_INITIALIZE_METHOD(tally, tally_dispatch_initialize);
BT_PLUGIN_SINK_COMPONENT_CLASS_FINALIZE_METHOD(tally, tally_dispatch_finalize);
BT_PLUGIN_SINK_COMPONENT_CLASS_GRAPH_IS_CONFIGURED_METHOD(tally, tally_dispatch_graph_is_configured);
