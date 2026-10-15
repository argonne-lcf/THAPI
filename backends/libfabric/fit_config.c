#include "fi_trace.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

struct fit_config fit_cfg;
int fit_armed;
_Atomic unsigned int fit_av_gen;

void fit_log(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	int n;

	if (!fit_cfg.verbose)
		return;

	n = snprintf(buf, sizeof(buf), "fi_trace[%d]: ", (int)getpid());
	va_start(ap, fmt);
	n += vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
	va_end(ap);
	if (n < 0)
		return;
	if ((size_t)n >= sizeof(buf) - 1)
		n = sizeof(buf) - 2;
	buf[n++] = '\n';
	(void)!write(STDERR_FILENO, buf, n);
}


static int env_int(const char *name, int dflt)
{
	const char *v = getenv(name);

	if (!v || !*v)
		return dflt;
	if (!strcasecmp(v, "yes") || !strcasecmp(v, "true") ||
	    !strcasecmp(v, "on"))
		return 1;
	if (!strcasecmp(v, "no") || !strcasecmp(v, "false") ||
	    !strcasecmp(v, "off"))
		return 0;
	return (int)strtol(v, NULL, 0);
}

static uint64_t env_u64(const char *name, uint64_t dflt)
{
	const char *v = getenv(name);

	return (v && *v) ? strtoull(v, NULL, 0) : dflt;
}

static void parse_ops(const char *spec)
{
	/* default: everything on */
	fit_cfg.trace_msg = fit_cfg.trace_tagged = fit_cfg.trace_rma = 1;

	if (!spec || !*spec || !strcasecmp(spec, "all"))
		return;

	fit_cfg.trace_msg = fit_cfg.trace_tagged = fit_cfg.trace_rma = 0;
	if (strcasestr(spec, "msg"))
		fit_cfg.trace_msg = 1;
	if (strcasestr(spec, "tag"))
		fit_cfg.trace_tagged = 1;
	if (strcasestr(spec, "rma"))
		fit_cfg.trace_rma = 1;
}

void fit_config_init(void)
{
	const char *dir = getenv("FI_TRACE_DIR");

	fit_cfg.enabled     = env_int("FI_TRACE_ENABLE", 1);
	fit_cfg.resolve     = env_int("FI_TRACE_RESOLVE", 1);
	fit_cfg.trace_rx    = env_int("FI_TRACE_RX", 1);
	fit_cfg.trace_srx   = env_int("FI_TRACE_SRX", 0);
	fit_cfg.trace_eagain = env_int("FI_TRACE_EAGAIN", 1);
	fit_cfg.trace_cq    = env_int("FI_TRACE_CQ", 1);
	fit_cfg.verbose     = env_int("FI_TRACE_VERBOSE", 0);
	fit_cfg.bufsz       = (size_t)env_u64("FI_TRACE_BUFSZ", 1u << 20);
	fit_cfg.max_records = env_u64("FI_TRACE_MAX_RECORDS", 0);
	parse_ops(getenv("FI_TRACE_OPS"));

	if (fit_cfg.bufsz < 4 * FIT_REC_MAX)
		fit_cfg.bufsz = 4 * FIT_REC_MAX;

	snprintf(fit_cfg.dir, sizeof(fit_cfg.dir), "%s",
		 (dir && *dir) ? dir : ".");

	if (gethostname(fit_cfg.host, sizeof(fit_cfg.host)) != 0)
		snprintf(fit_cfg.host, sizeof(fit_cfg.host), "unknown");
	fit_cfg.host[sizeof(fit_cfg.host) - 1] = '\0';
	fit_sanitize(fit_cfg.host, &fit_cfg.host_len);
}