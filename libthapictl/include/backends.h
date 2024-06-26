#pragma once

#include <lttng/lttng.h>

void thapi_cuda_init(struct lttng_handle *h, const char *channel_name);
void thapi_cuda_enable_tracing_events(struct lttng_handle *h, const char *channel_name);
void thapi_cuda_disable_tracing_events(struct lttng_handle *h, const char *channel_name);

void thapi_opencl_init(struct lttng_handle *h, const char *channel_name);
void thapi_opencl_enable_tracing_events(struct lttng_handle *h, const char *channel_name);
void thapi_opencl_disable_tracing_events(struct lttng_handle *h, const char *channel_name);
