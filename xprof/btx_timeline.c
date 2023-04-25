#include <babeltrace2/babeltrace.h>
#include "btx_timeline.h"

/* Mandatory */
BT_PLUGIN_MODULE();

/* Define the `timeline` plugin */
BT_PLUGIN(timeline);

//~ ~ ~
// Sink Timeline
//~ ~ ~

BT_PLUGIN_SINK_COMPONENT_CLASS(timeline, timeline_dispatch_consume);

BT_PLUGIN_SINK_COMPONENT_CLASS_INITIALIZE_METHOD(timeline, timeline_dispatch_initialize);
BT_PLUGIN_SINK_COMPONENT_CLASS_FINALIZE_METHOD(timeline, timeline_dispatch_finalize);
BT_PLUGIN_SINK_COMPONENT_CLASS_GRAPH_IS_CONFIGURED_METHOD(timeline, timeline_dispatch_graph_is_configured);
