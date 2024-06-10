#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lttng/lttng.h>


#define THAPI_LTTNG_SESSION_ID_NAME "THAPI_LTTNG_SESSION_ID"
#define THAPI_CTL_LOG_LEVEL_NAME "THAPI_CTL_LOG_LEVEL"

#define THAPI_CTL_LOG_LEVEL_ERROR 1
#define THAPI_CTL_LOG_LEVEL_INFO 2
#define THAPI_CTL_LOG_LEVEL_WARN 3
#define THAPI_CTL_LOG_LEVEL_DEBUG 4

#define THAPI_CTL_DEFAULT_LOG_LEVEL THAPI_CTL_LOG_LEVEL_ERROR


static const char* thapi_ctl_level_str[5] = {
  "",
  "ERROR",
  "INFO",
  "WARN",
  "DEBUG"
};


int _thapi_ctl_log_level() {
  static int level = -1;
  if (level < 0) {
    const char *log_level_str = getenv(THAPI_CTL_LOG_LEVEL_NAME);
    if (log_level_str == NULL || strlen(log_level_str) == 0) {
      level = THAPI_CTL_DEFAULT_LOG_LEVEL;
    } else {
      char *endptr;
      level = strtol(log_level_str, &endptr, 10);
      if (*endptr != '\0') {
        level = 0;
      }
    }
  }
  return level;
}


void _thapi_ctl_log(int log_level, const char *fmt, ...) {
  assert(log_level > 1 && log_level <= THAPI_CTL_LOG_LEVEL_DEBUG);
  int config_level = _thapi_ctl_log_level();
  if (log_level <= config_level) {
    fprintf(stderr, "libthapi-ctl[%s]: ", thapi_ctl_level_str[log_level]);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
  }
}


int thapi_start_tracing() {
  const char *session_id = getenv(THAPI_LTTNG_SESSION_ID_NAME);
  if (session_id == NULL) {
    // TODO: silently ignore this error? Apps should be able to be run with or
    // without iprof
    _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_WARN,
                   "Failed to start tracing on session, missing env var '%s'",
                   THAPI_LTTNG_SESSION_ID_NAME);
    return -LTTNG_ERR_SESS_NOT_FOUND;
  }
  const int rval = lttng_start_tracing(session_id);
  if (rval < 0) {
    if (rval == -LTTNG_ERR_TRACE_ALREADY_STARTED) {
      _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_INFO,
                     "Session already started (id '%s')", session_id);
    } else {
      _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                     "Failed to stop tracing on session '%s': %d",
                     session_id, rval);
    }
  } else {
    _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG,
                   "Starting session '%s", session_id);
  }
  return rval;
}


int thapi_stop_tracing() {
  const char *session_id = getenv(THAPI_LTTNG_SESSION_ID_NAME);
  if (session_id == NULL) {
    _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_WARN,
                   "Failed to start tracing on session, missing env var '%s'",
                   THAPI_LTTNG_SESSION_ID_NAME);
    return -LTTNG_ERR_SESS_NOT_FOUND;
  }
  const int rval = lttng_stop_tracing(session_id);
  // TODO: do we really want to ignore double stop?
  if (rval < 0) {
    if (rval == -LTTNG_ERR_TRACE_ALREADY_STOPPED) {
      _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_INFO,
                     "Session already stopped (id '%s')", session_id);
    } else {
      _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                     "Failed to stop tracing on session '%s': %d",
                     session_id, rval);
    }
  } else {
    _thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG,
                   "Stopping session '%s", session_id);
  }
  return rval;
}
