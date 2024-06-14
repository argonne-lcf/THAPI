#pragma once

#define THAPI_CTL_LOG_LEVEL_ERROR 1
#define THAPI_CTL_LOG_LEVEL_INFO 2
#define THAPI_CTL_LOG_LEVEL_WARN 3
#define THAPI_CTL_LOG_LEVEL_DEBUG 4

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize bookkeeping events for enabled backends, if run with iprof
 *
 * Does nothing if not run with iprof. This is detected using the
 * THAPI_LTTNG_SESSION_ID environment variable.
 *
 * @param enable_tracing: if false, enable only bookkeeping events, otherwise
 *  enable all events
 *
 * @return 0 on success
 */
int thapi_ctl_init();


/**
 * Start tracing host and device events, when application is run with iprof.
 *
 * Does nothing if not run with iprof. This is detected using the
 * THAPI_LTTNG_SESSION_ID environment variable.
 *
 * @return 0 on success
 */
int thapi_ctl_start();

/**
 * Stop tracing host and device events, when application is run with iprof.
 *
 * Does nothing if not run with iprof. This is detected using the
 * THAPI_LTTNG_SESSION_ID environment variable.
 *
 * @return 0 on success
 */
int thapi_ctl_stop();


/**
 * Destroy any lttng handles associated with thapi-ctl.
 *
 * @return 0 on success
 */
void thapi_ctl_destroy();


/**
 * Log to stderr according to configured log level.
 *
 * Log level is set with THAPI_CTL_LOG_LEVEL env variable.
 *
 * @param log_level: one of THAPI_CTL_LOG_LEVEL_(DEBUG|INFO|WARN|ERROR)
 * @param fmt: printf style format
 * @param ...: printf format arguments
 */
void thapi_ctl_log(int log_level, const char *fmt, ...);


void thapi_ctl_print_events();


#ifdef __cplusplus
}
#endif
