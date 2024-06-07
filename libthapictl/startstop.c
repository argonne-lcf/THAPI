#include <lttng/lttng-error.h>
#include <stdlib.h>
#include <stdio.h>

#include <lttng/lttng.h>

#define THAPI_LTTNG_SESSION_ID_NAME "THAPI_LTTNG_SESSION_ID"


int thapi_start_tracing() {
  const char *session_id = getenv(THAPI_LTTNG_SESSION_ID_NAME);
  if (session_id == NULL) {
    // TODO: silently ignore this error? Apps should be able to be run with or
    // without iprof
    fprintf(stderr, "Failed to start tracing on session, missing env var '%s'\n",
            THAPI_LTTNG_SESSION_ID_NAME);
    return -LTTNG_ERR_SESS_NOT_FOUND;
  }
  const int rval = lttng_start_tracing(session_id);
  // TODO: do we really want to ignore double stop?
  if (rval < 0 && rval != -LTTNG_ERR_TRACE_ALREADY_STARTED) {
    fprintf(stderr, "Failed to start tracing on session '%s': %d\n", session_id,
            rval);
  }
  return rval;
}


int thapi_stop_tracing() {
  const char *session_id = getenv(THAPI_LTTNG_SESSION_ID_NAME);
  if (session_id == NULL) {
    fprintf(stderr, "Failed to stop tracing on session, missing env var '%s'\n",
            THAPI_LTTNG_SESSION_ID_NAME);
    return -LTTNG_ERR_SESS_NOT_FOUND;
  }
  const int rval = lttng_stop_tracing(session_id);
  // TODO: do we really want to ignore double stop?
  if (rval < 0 && rval != -LTTNG_ERR_TRACE_ALREADY_STOPPED) {
    fprintf(stderr, "Failed to stop tracing on session '%s': %d\n", session_id,
            rval);
  }
  return rval;
}
