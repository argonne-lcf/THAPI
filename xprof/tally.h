#pragma once 

#include <babeltrace2/babeltrace.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sink component's private data */
struct tally_dispatch {
    bt_message_iterator *message_iterator;
};

bt_component_class_sink_consume_method_status tally_dispatch_consume(
                bt_self_component_sink *self_component_sink);

bt_component_class_initialize_method_status tally_dispatch_initialize(
        bt_self_component_sink *self_component_sink,
        bt_self_component_sink_configuration *configuration,
        const bt_value *params, void *initialize_method_data);

void tally_dispatch_finalize(bt_self_component_sink *self_component_sink);


bt_component_class_sink_graph_is_configured_method_status
tally_dispatch_graph_is_configured(bt_self_component_sink *self_component_sink);

#ifdef __cplusplus
}
#endif
