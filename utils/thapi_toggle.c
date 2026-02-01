#include "thapi.h"
#include "thapi_toggle_tracepoints.h"

#ifndef LTTNG_UST_CONSTRUCTOR_PRIO
#error "LTTNG_UST_CONSTRUCTOR_PRIO is not defined."
#endif

void thapi_start(void) { tracepoint(lttng_ust_toggle, start); }

void thapi_stop(void) { tracepoint(lttng_ust_toggle, stop); }

void __attribute__((constructor(LTTNG_UST_CONSTRUCTOR_PRIO + 1)))
thapi_auto_stop(void) { tracepoint(lttng_ust_toggle, auto_stop); }
