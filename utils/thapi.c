#include "thapi_tracepoints.h"

void thapi_stop() __attribute__((constructor));

void thapi_start(void) {
  tracepoint(lttng_ust_toggle, start);
}

void thapi_stop(void) {
  tracepoint(lttng_ust_toggle, stop);
}
