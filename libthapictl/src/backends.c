#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lttng/lttng.h>

#include "thapi.h"
#include "thapi-ctl.h"


enum tracing_mode_e {
  THAPI_CTL_TRACING_MODE_UNSET=0,
  THAPI_CTL_TRACING_MODE_MINIMAL=1,
  THAPI_CTL_TRACING_MODE_DEFAULT=2,
  THAPI_CTL_TRACING_MODE_FULL=3,
};
typedef enum tracing_mode_e tracing_mode_t;


#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(*a));


/*
 *  "lttng_ust_cuda:cuDriverGetVersion_entry",
  "lttng_ust_cuda:cuDriverGetVersion_exit",
  "lttng_ust_cuda:cuDriverGetAttribute_entry",
  "lttng_ust_cuda:cuDriverGetAttribute_exit",
  "lttng_ust_cuda:cuInit_entry",
  "lttng_ust_cuda:cuInit_exit",
  "lttng_ust_cuda:cuGetExportTable_entry",
  "lttng_ust_cuda:cuGetExportTable_exit",
  "lttng_ust_cuda:cuCtxGetCurrent_entry",
  "lttng_ust_cuda:cuCtxGetCurrent_exit",
  */

static char* cuda_bookkeeping_events[] = {
  "lttng_ust_cuda:cuGetProcAddress_v2_entry",
  "lttng_ust_cuda:cuGetProcAddress_v2_exit",
  "lttng_ust_cuda:cuCtxCreate_entry",
  "lttng_ust_cuda:cuCtxCreate_exit",
  "lttng_ust_cuda:cuCtxCreate_v2_entry",
  "lttng_ust_cuda:cuCtxCreate_v2_exit",
  "lttng_ust_cuda:cuCtxCreate_v3_entry",
  "lttng_ust_cuda:cuCtxCreate_v3_exit",
  "lttng_ust_cuda:cuCtxDestroy_entry",
  "lttng_ust_cuda:cuCtxDestroy_exit",
  "lttng_ust_cuda:cuCtxDestroy_v2_entry",
  "lttng_ust_cuda:cuCtxDestroy_v2_exit",
  "lttng_ust_cuda:cuCtxSetCurrent_entry",
  "lttng_ust_cuda:cuCtxSetCurrent_exit",
  "lttng_ust_cuda:cuCtxPushCurrent_entry",
  "lttng_ust_cuda:cuCtxPushCurrent_exit",
  "lttng_ust_cuda:cuCtxPushCurrent_v2_entry",
  "lttng_ust_cuda:cuCtxPushCurrent_v2_exit",
  "lttng_ust_cuda:cuCtxPopCurrent_entry",
  "lttng_ust_cuda:cuCtxPopCurrent_exit",
  "lttng_ust_cuda:cuCtxPopCurrent_v2_entry",
  "lttng_ust_cuda:cuCtxPopCurrent_v2_exit",
  "lttng_ust_cuda:cuDevicePrimaryCtxRetain_entry",
  "lttng_ust_cuda:cuDevicePrimaryCtxRetain_exit",
  "lttng_ust_cuda:cuStreamCreate_entry",
  "lttng_ust_cuda:cuStreamCreate_exit",
  "lttng_ust_cuda:cuStreamCreateWithPriority_entry",
  "lttng_ust_cuda:cuStreamCreateWithPriority_exit",
  "lttng_ust_cuda:cuModuleGetFunction_entry",
  "lttng_ust_cuda:cuModuleGetFunction_exit",
  "lttng_ust_cuda_profiling:event_profiling_results",
};


// TODO: encapsulate all env vars into a single struct
static tracing_mode_t thapi_ctl_tracing_mode() {
  static tracing_mode_t mode = THAPI_CTL_TRACING_MODE_UNSET;
  if (mode == THAPI_CTL_TRACING_MODE_UNSET) {
    const char *mode_env = getenv("LTTNG_UST_TRACING_MODE");
    if (mode_env == NULL) {
      mode = THAPI_CTL_TRACING_MODE_DEFAULT;
    } else if (strcmp(mode_env, "minimal") == 0) {
      mode = THAPI_CTL_TRACING_MODE_MINIMAL;
    } else if (strcmp(mode_env, "full") == 0) {
      mode = THAPI_CTL_TRACING_MODE_FULL;
    } else if (strcmp(mode_env, "default") == 0) {
      mode = THAPI_CTL_TRACING_MODE_DEFAULT;
    } else {
      thapi_ctl_log(THAPI_CTL_LOG_LEVEL_WARN,
                    "Unknown tracing mode: %s, using default", mode_env);
      mode = THAPI_CTL_TRACING_MODE_DEFAULT;
    }
  }
  return mode;
}


static int thapi_ctl_profiling() {
  static int profiling = -1;
  if (profiling == -1) {
    const char *profiling_env = getenv("LTTNG_UST_PROFILING");
    if (profiling_env == NULL || strcmp(profiling_env, "1") == 0) {
      profiling = 1;
    } else {
      profiling = 0;
    }
  }
  return profiling;
}


static char* cuda_tracing_events[] = {
  "lttng_ust_cuda:*",
  "lttng_ust_cuda_properties",
  "lttng_ust_cuda_profiling:event_profiling",
};


static inline int is_wildcard(const char *event_pattern) {
  return (strchr(event_pattern, '*') != NULL);
}


static void thapi_enable_events_by_pattern(struct lttng_handle* handle,
                                           struct lttng_event* ev,
                                           int n_patterns,
                                           char** event_patterns,
                                           const char* channel_name,
                                           int exclusion_count,
                                           char **exclusion_list) {
  memset(ev, 0, sizeof(*ev));
  ev->type = LTTNG_EVENT_TRACEPOINT;

  for (int i = 0; i < n_patterns; i++) {
    const char *pattern = event_patterns[i];
    // thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG, "Enabling event '%s' on channel '%s'",
    //               pattern, channel_name);
    strncpy(ev->name, pattern, LTTNG_SYMBOL_NAME_LEN);
    ev->name[LTTNG_SYMBOL_NAME_LEN - 1] = '\0';
    int rval = 0;
    if (exclusion_count > 0 && is_wildcard(pattern)) {
      rval = lttng_enable_event_with_exclusions(handle, ev, channel_name, NULL,
                                                exclusion_count, exclusion_list);
    } else {
      rval = lttng_enable_event_with_exclusions(handle, ev, channel_name, NULL,
                                                0, NULL);
    }

    if (rval < 0 && rval != -LTTNG_ERR_UST_EVENT_ENABLED) {
      thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                    "Failed to enable event '%s': %d",
                    pattern, rval);
    }
  }
}


static void thapi_disable_events_by_pattern(struct lttng_handle* handle,
                                            struct lttng_event* ev,
                                            int n_patterns,
                                            char** event_patterns,
                                            const char* channel_name) {
  memset(ev, 0, sizeof(*ev));
  ev->type = LTTNG_EVENT_ALL; // match any event type
  ev->loglevel = -1; // match any log level

  for (int i = 0; i < n_patterns; i++) {
    const char *pattern = event_patterns[i];
    // thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG, "Disabling event '%s' on channel '%s'",
    //               pattern, channel_name);
    strncpy(ev->name, pattern, LTTNG_SYMBOL_NAME_LEN);
    ev->name[LTTNG_SYMBOL_NAME_LEN - 1] = '\0';
    int rval = lttng_disable_event_ext(handle, ev, channel_name, NULL);
    if (rval < 0) {
      thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                    "Failed to disable event '%s': %d",
                    pattern, rval);
    }
  }
}


void thapi_cuda_init(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  int n_bookkeeping_events = ARRAY_LENGTH(cuda_bookkeeping_events);
  thapi_enable_events_by_pattern(h, ev, n_bookkeeping_events, cuda_bookkeeping_events,
                                 channel_name, 0, NULL);

  lttng_event_destroy(ev);
}


void thapi_cuda_enable_tracing_events(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  int n_bookkeeping_events = ARRAY_LENGTH(cuda_bookkeeping_events);
  int n_tracing_events = ARRAY_LENGTH(cuda_tracing_events);
  thapi_enable_events_by_pattern(h, ev, n_tracing_events, cuda_tracing_events, channel_name,
                                 n_bookkeeping_events, cuda_bookkeeping_events);

  lttng_event_destroy(ev);
}


void thapi_cuda_disable_tracing_events(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  int n_tracing_events = sizeof(cuda_tracing_events)
                         / sizeof(*cuda_tracing_events);
  thapi_disable_events_by_pattern(h, ev, n_tracing_events,
                                  cuda_tracing_events, channel_name);

  lttng_event_destroy(ev);
}


static char* opencl_bookkeeping_events[] = {
  "lttng_ust_opencl:clGetDeviceIDs",
  "lttng_ust_opencl:clCreatSubDevices",
  "lttng_ust_opencl:clCreateCommandQueue",
  "lttng_ust_opencl_arguments:kernel_info",
  "lttng_ust_opencl_profiling:event_profiling_results"
};

static char* opencl_tracing_default_events[] = {
  "lttng_ust_opencl_devices:*",
  "lttng_ust_opencl_arguments:*",
  "lttng_ust_opencl_build:infos*",
};

static char* opencl_tracing_default_exclude_events[] = {
  "lttng_ust_opencl:clSetKernelArg*"
  "lttng_ust_opencl:clGetKernelArg*",
  "lttng_ust_opencl:clSetKernelExecInfo*"
  "lttng_ust_opencl:clGetKernelInfo*",
  "lttng_ust_opencl:clGetMemAllocInfoINTEL*"
};

static char* opencl_profiling_events[] = {
  "lttng_ust_opencl_profiling:event_profiling",
};


void thapi_opencl_init(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  int n_bookkeeping_events = ARRAY_LENGTH(opencl_bookkeeping_events);
  thapi_enable_events_by_pattern(h, ev, n_bookkeeping_events, opencl_bookkeeping_events,
                                 channel_name, 0, NULL);

  lttng_event_destroy(ev);
}


void thapi_opencl_enable_tracing_events(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  tracing_mode_t mode = thapi_ctl_tracing_mode();

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  char *host_events_pattern = "lttng_ust_opencl:*";
  if (mode == THAPI_CTL_TRACING_MODE_MINIMAL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_WARN,
                  "mode minimal no supproted by opencl backend, using default mode");
    mode = THAPI_CTL_TRACING_MODE_DEFAULT;
  }

  if (mode == THAPI_CTL_TRACING_MODE_FULL) {
    thapi_enable_events_by_pattern(h, ev, 1, &host_events_pattern, channel_name,
                                   0, NULL);
  } else { // MODE_DEFAULT, add same pattern but with exclusion list
    int n_exclude_default = ARRAY_LENGTH(opencl_tracing_default_exclude_events);
    thapi_enable_events_by_pattern(h, ev, 1, &host_events_pattern, channel_name,
                                   n_exclude_default,
                                   opencl_tracing_default_exclude_events);
  }

  int n_tracing_default = ARRAY_LENGTH(opencl_tracing_default_events);
  thapi_enable_events_by_pattern(h, ev, n_tracing_default, opencl_tracing_default_events,
                                 channel_name, 0, NULL);

  if (thapi_ctl_profiling()) {
    int n_profiling = ARRAY_LENGTH(opencl_profiling_events);
    thapi_enable_events_by_pattern(h, ev, n_profiling, opencl_profiling_events,
                                   channel_name, 0, NULL);
  }

  lttng_event_destroy(ev);
}


void thapi_opencl_disable_tracing_events(struct lttng_handle *h, const char *channel_name) {
  int rval = 0;

  struct lttng_event *ev = lttng_event_create();
  if (ev == NULL) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Error creating event: %d", rval);
    return;
  }

  int n_tracing_default_events = sizeof(opencl_tracing_default_events)
                               / sizeof(*opencl_tracing_default_events);
  thapi_disable_events_by_pattern(h, ev, n_tracing_default_events,
                                  opencl_tracing_default_events, channel_name);

  char *host_events_pattern = "lttng_ust_opencl:*";
  thapi_disable_events_by_pattern(h, ev, 1, &host_events_pattern, channel_name);

  if (thapi_ctl_profiling()) {
    int n_profiling = ARRAY_LENGTH(opencl_profiling_events);
    thapi_disable_events_by_pattern(h, ev, n_profiling, opencl_profiling_events,
                                    channel_name);
  }

  lttng_event_destroy(ev);
}
