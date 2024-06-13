#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start tracing host and device events, when application is run with iprof.
 *
 * Does nothing if not run with iprof. This is detected using the
 * THAPI_LTTNG_SESSION_ID environment variable.
 *
 * @return 0 on success
 */
int thapi_start_tracing();

/**
 * Stop tracing host and device events, when application is run with iprof.
 *
 * NOTE: must not be called until after any device runtime initialization is
 * complete, in case dynamic symbols are loaded. For CUDA, calling `cudaFree(0)`
 * will force all symbols be loaded and not doing anything else, and make it
 * safe to call thapi_stop_tracing.
 *
 * Does nothing if not run with iprof. This is detected using the
 * THAPI_LTTNG_SESSION_ID environment variable.
 *
 * @return 0 on success
 */
int thapi_stop_tracing();
 
#ifdef __cplusplus
}
#endif
