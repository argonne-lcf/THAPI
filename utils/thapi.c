#include "thapi_tracepoints.h"
#include "thapi.h"

void thapi_start(void) {
  tracepoint(lttng_ust_toggle, start);
}

void thapi_stop(void) {
  tracepoint(lttng_ust_toggle, stop);
}
