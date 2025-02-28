#include "thapi_toggle_tracepoints.h"

void thapi_start(void) {
  tracepoint(lttng_ust_thapi, start);
}

void thapi_stop(void) {
  tracepoint(lttng_ust_thapi, stop);
}
