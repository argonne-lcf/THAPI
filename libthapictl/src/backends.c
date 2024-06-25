#include <stdio.h>
#include <string.h>

#include <lttng/lttng.h>

#include "thapi.h"
#include "thapi-ctl.h"


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


static const char* cuda_tracing_events[] = {
  "lttng_ust_cuda:*",
  "lttng_ust_cuda_properties",
  "lttng_ust_cuda_profiling:event_profiling",
};


static int inline is_wildcard(const char *event_pattern) {
  return (strchr(event_pattern, '*') != NULL);
}


static void thapi_enable_events_by_pattern(struct lttng_handle* handle,
                                           struct lttng_event* ev,
                                           const char* event_pattern,
                                           const char* channel_name,
                                           int exclusion_count,
                                           char **exclusion_list) {
  // thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG, "Enabling event '%s' on channel '%s'",
  //               event_pattern, channel_name);
  memset(ev, 0, sizeof(*ev));
  strncpy(ev->name, event_pattern, LTTNG_SYMBOL_NAME_LEN);
  ev->name[LTTNG_SYMBOL_NAME_LEN - 1] = '\0';
  ev->type = LTTNG_EVENT_TRACEPOINT;
  int rval = lttng_enable_event_with_exclusions(handle, ev, channel_name, NULL,
                                                exclusion_count, exclusion_list);
  if (rval < 0 && rval != -LTTNG_ERR_UST_EVENT_ENABLED) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Failed to enable event '%s': %d",
                  event_pattern, rval);
  }
}


static void thapi_disable_events_by_pattern(struct lttng_handle* handle,
                                            struct lttng_event* ev,
                                            const char* event_pattern,
                                            const char* channel_name) {
  // thapi_ctl_log(THAPI_CTL_LOG_LEVEL_DEBUG, "Disabling event '%s' on channel '%s'",
  //               event_pattern, channel_name);
  memset(ev, 0, sizeof(*ev));
  strncpy(ev->name, event_pattern, LTTNG_SYMBOL_NAME_LEN);
  ev->name[LTTNG_SYMBOL_NAME_LEN - 1] = '\0';
  ev->type = LTTNG_EVENT_ALL; // match any event type
  ev->loglevel = -1; // match any log level
  int rval = lttng_disable_event_ext(handle, ev, channel_name, NULL);
  if (rval < 0) {
    thapi_ctl_log(THAPI_CTL_LOG_LEVEL_ERROR,
                  "Failed to disable event '%s': %d",
                  event_pattern, rval);
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
  for (int i = 0; i < n_bookkeeping_events; i++) {
    thapi_enable_events_by_pattern(h, ev, cuda_bookkeeping_events[i],
                                   channel_name, 0, NULL);
  }

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
  for (int i = 0; i < n_tracing_events; i++) {
    const char *event_pattern = cuda_tracing_events[i];
    // exclude bookkeeping events if there is a wildcard in the pattern
    if (is_wildcard(event_pattern)) {
      thapi_enable_events_by_pattern(h, ev, event_pattern, channel_name,
                                     n_bookkeeping_events, cuda_bookkeeping_events);
    } else {
      thapi_enable_events_by_pattern(h, ev, event_pattern, channel_name,
                                     0, NULL);
    }
  }

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
  for (int i = 0; i < n_tracing_events; i++) {
    thapi_disable_events_by_pattern(h, ev, cuda_tracing_events[i], channel_name);
  }

  lttng_event_destroy(ev);
}


static char* opencl_bookkeeping_events[] = {
  "lttng_ust_opencl:clGetDeviceIDs",
  "lttng_ust_opencl:clCreatSubDevices",
  "lttng_ust_opencl:clCreateCommandQueue",
  "lttng_ust_opencl_arguments:kernel_info",
};


static char* opencl_tracing_events[] = {
  "lttng_ust_opencl:*",
  "lttng_ust_opencl_profiling:*",
  "lttng_ust_opencl_devices:*",
  "lttng_ust_opencl_arguments:*",
  "lttng_ust_opencl_build:infos*",
};


/*
  when 'full'
    exec("#{lttng_enable} lttng_ust_opencl:*")
    exec("#{lttng_enable} lttng_ust_opencl_profiling:*") if profiling
    exec("#{lttng_enable} lttng_ust_opencl_devices:*")
    exec("#{lttng_enable} lttng_ust_opencl_arguments:*")
    exec("#{lttng_enable} lttng_ust_opencl_build:infos*")
  when 'default'
    exec("#{lttng_enable} lttng_ust_opencl_profiling:*") if profiling
    exec("#{lttng_enable} lttng_ust_opencl_devices:*")
    exec("#{lttng_enable} lttng_ust_opencl_arguments:*")
    exec("#{lttng_enable} lttng_ust_opencl_build:infos*")
    # Wildcard using the * character are supported at the end of tracepoint names.
    #   https://lttng.org/man/1/lttng-enable-event/v2.8/#doc-_understanding_event_rule_conditions
    # Disable-event doesn't have wildcards
    # So we enable and disable on the same line
    opencl_disable = ['lttng_ust_opencl:clSetKernelArg*', 'lttng_ust_opencl:clGetKernelArg*',
                      'lttng_ust_opencl:clSetKernelExecInfo*', 'lttng_ust_opencl:clGetKernelInfo*',
                      'lttng_ust_opencl:clGetMemAllocInfoINTEL*']

    exec("#{lttng_enable} lttng_ust_opencl:* -x #{opencl_disable.join(',')}")
  when 'minimal'
    LOGGER.debug("Tracing mode #{tracing_mode} not supported for OpenCL")
  else
    raise("Tracing mode #{tracing_mode} not supported")
  end */
