#include "thapi_profiler_tracepoints.h"

void thapi_profiler_stop() __attribute__((constructor));

void thapi_profiler_start(void) {
  tracepoint(lttng_ust_profiler, start);
}

void thapi_profiler_stop(void) {
  tracepoint(lttng_ust_profiler, stop);
}
