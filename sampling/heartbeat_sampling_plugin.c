#include "sampling.h"
#include "thapi_sampling.h"
#include <time.h>

static void thapi_sampling_heartbeat() { do_tracepoint(lttng_ust_sampling, heartbeat, 16); }
static void thapi_sampling_heartbeat2() { do_tracepoint(lttng_ust_sampling, heartbeat2); }

static void *plugin_handle_heartbeat = NULL;
static void *plugin_handle_heartbeat2 = NULL;

void thapi_initialize_sampling_plugin(void) {
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT")) {
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 100000000;
    plugin_handle_heartbeat = thapi_register_sampling(&thapi_sampling_heartbeat, &interval);
  }
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT2")) {
    struct timespec interval;
    interval.tv_sec = 2;
    interval.tv_nsec = 30000000;
    plugin_handle_heartbeat2 = thapi_register_sampling(&thapi_sampling_heartbeat2, &interval);
  }
}

void thapi_finalize_sampling_plugin(void) {
  if (plugin_handle_heartbeat == NULL) {
    do_tracepoint(lttng_ust_sampling, heartbeat, 32);
    thapi_unregister_sampling(plugin_handle_heartbeat);
  }
  if (plugin_handle_heartbeat2 == NULL)
    thapi_unregister_sampling(plugin_handle_heartbeat2);
}
