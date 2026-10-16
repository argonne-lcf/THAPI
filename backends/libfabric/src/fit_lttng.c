/*
 * fit_lttng.c -- replaces fit_writer.c.
 *
 * The standalone tracer wrote sanitised CSV into a per-thread buffer and
 * flushed it to one file per thread, then joined the pieces with
 * tools/fitrace-merge.  Under THAPI, LTTng owns all of that.  What goes away:
 *
 *   - the per-thread buffer, its mutex, the flush-on-exit destructor, and
 *     FI_TRACE_DIR / FI_TRACE_BUFSZ / FI_TRACE_MAX_RECORDS
 *   - the hostname, pid and tid columns: LTTng adds vpid and vtid as channel
 *     contexts and hostname as a trace environment entry, so the hot path
 *     never touches them
 *   - the cached getpid() and its pthread_atfork re-read, which existed only
 *     because the pid was a column
 *   - the timestamp: LTTng stamps every event from the monotonic clock
 *   - the CSV quoting in fit_sanitize(); CTF strings are length-delimited.
 *     fit_sanitize() is kept, because an address string with an embedded
 *     newline would still make the pretty-printed output unreadable, but it
 *     now only strips control characters.
 *
 * What is deliberately kept, because fit_hook.c and fit_addr.c call it and
 * should not need editing for this:
 *
 *   - the fit_record() / fit_record_cq() signatures
 *   - fit_cfg, fit_config_init(), fit_log(), fit_armed
 *   - fit_now_us(), now a stub returning 0 (see below)
 *
 * Environment variables are renamed FI_TRACE_* -> LTTNG_UST_FI_* to match the
 * house convention.  The ones that configured the CSV writer are gone.
 */

#define _GNU_SOURCE

#include "fi_trace.h"
#include "libfabric_tracepoints.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rdma/fi_errno.h>

struct fit_config fit_cfg;
int fit_armed;
_Atomic unsigned int fit_av_gen = 0;

/* ------------------------------------------------------------------ */
/* logging (stderr, not the trace -- this is for debugging the tracer) */
/* ------------------------------------------------------------------ */

void fit_log(const char *fmt, ...)
{
	va_list ap;

	if (!fit_cfg.verbose)
		return;

	fprintf(stderr, "[fi_trace] ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

/* ------------------------------------------------------------------ */
/* configuration                                                      */
/* ------------------------------------------------------------------ */

static int env_int(const char *name, int dflt)
{
	const char *v = getenv(name);

	return v ? atoi(v) : dflt;
}

/*
 * LTTNG_UST_FI_OPS selects which op families get their vtable swapped at all.
 * This is not the same thing as enabling or disabling tracepoints: a slot we
 * never swap costs the application literally nothing, where a disabled
 * tracepoint still costs the branch.  For an application that only does tagged
 * messaging, "LTTNG_UST_FI_OPS=tagged" is measurably cheaper than filtering on
 * the LTTng side.
 */
static void parse_ops(const char *v)
{
	if (!v || !*v) {
		fit_cfg.trace_msg = fit_cfg.trace_tagged = fit_cfg.trace_rma = 1;
		return;
	}

	fit_cfg.trace_msg = fit_cfg.trace_tagged = fit_cfg.trace_rma = 0;

	if (strstr(v, "msg"))
		fit_cfg.trace_msg = 1;
	if (strstr(v, "tagged"))
		fit_cfg.trace_tagged = 1;
	if (strstr(v, "rma"))
		fit_cfg.trace_rma = 1;
}

void fit_config_init(void)
{
	fit_cfg.enabled      = env_int("LTTNG_UST_FI_ENABLE", 1);
	fit_cfg.resolve      = env_int("LTTNG_UST_FI_RESOLVE", 1);
	fit_cfg.trace_rx     = env_int("LTTNG_UST_FI_RX", 1);
	fit_cfg.trace_srx    = env_int("LTTNG_UST_FI_SRX", 0);
	fit_cfg.trace_eagain = env_int("LTTNG_UST_FI_EAGAIN", 1);
	fit_cfg.trace_cq     = env_int("LTTNG_UST_FI_CQ", 1);
	fit_cfg.verbose      = env_int("LTTNG_UST_FI_VERBOSE", 0);
	parse_ops(getenv("LTTNG_UST_FI_OPS"));

	/*
	 * fit_cfg.host / host_len / dir / bufsz / max_records are no longer
	 * read.  They stay in struct fit_config only so that fi_trace.h does
	 * not have to change; drop them once you are sure nothing else refers
	 * to them.
	 */
}

/* ------------------------------------------------------------------ */
/* string hygiene                                                     */
/* ------------------------------------------------------------------ */

/*
 * CTF strings are length-delimited, so commas and quotes no longer need
 * escaping.  Control characters still do: they would corrupt the aligned
 * columns of `iprof -t` output and, in the case of a stray NUL, truncate the
 * field.  Measured once where the string is produced, as before, so the hot
 * path only memcpys.
 */
void fit_sanitize(char *s, size_t *len_out)
{
	size_t n = 0;

	if (!s) {
		if (len_out)
			*len_out = 0;
		return;
	}

	for (; s[n]; n++) {
		if ((unsigned char)s[n] < 0x20 || (unsigned char)s[n] == 0x7f)
			s[n] = '_';
	}

	if (len_out)
		*len_out = n;
}

/*
 * Kept so fit_cq_report() in fit_hook.c compiles unchanged.  LTTng timestamps
 * every event itself from the monotonic clock, with better resolution than
 * clock_gettime() in userspace gives you after the syscall, so there is
 * nothing for this to do.
 *
 * Note the consequence for completions: a single fi_cq_read() returning a
 * burst of N entries used to stamp all N with one fit_now_us() value, which
 * was honest about the batch having no internal resolution.  LTTng will now
 * stamp them microseconds apart as the loop runs.  Treat sub-microsecond
 * spacing between completions from one reap as an artifact of the loop, not a
 * real arrival pattern.
 */
uint64_t fit_now_us(void)
{
	return 0;
}

/* ------------------------------------------------------------------ */
/* data-path records                                                  */
/* ------------------------------------------------------------------ */

/*
 * Entry marker.  Cheap on purpose: no AV lookup, no iov summation, no
 * sanitisation.  Its job is to make a hang or a crash inside the provider
 * visible as an entry with no matching exit, per AGEND_SKILLS.md.  All the
 * payload rides on the exit event, where it was already being computed before
 * this port.
 */
void fit_record_entry(struct fit_ep *h, const char *op, char dir,
		      const void *context)
{
	if (dir == 'R' && !fit_cfg.trace_rx)
		return;

	tracepoint(lttng_ust_fi, op_entry, op, h->ep_id, context);
}

void fit_record(struct fit_ep *h, const char *op, char dir, fi_addr_t peer,
		size_t bytes, size_t iov_count, ssize_t ret,
		const void *context)
{
	char scratch[FIT_ADDR_MAX];
	const char *peer_str;
	size_t peer_len = 0;

	if (dir == 'R' && !fit_cfg.trace_rx)
		return;

	/*
	 * A -FI_EAGAIN return means the provider refused the operation and the
	 * application will retry; no bytes moved.  Applications that spin on a
	 * busy endpoint can generate thousands of these, which swamps the real
	 * transfers, so they can be dropped.
	 *
	 * Dropping the exit here leaves the matching op_entry unpaired, and the
	 * analysis side discards an entry that never closes.  That is the
	 * intended behaviour, but it does mean LTTNG_UST_FI_EAGAIN=0 makes the
	 * entry/exit counts in a raw trace dump disagree -- do not go looking
	 * for a bug there.
	 */
	if (ret == -FI_EAGAIN && !fit_cfg.trace_eagain)
		return;

	if (!h->src_valid)
		fit_ep_resolve_src(h);

	peer_str = fit_ep_peer_str(h, peer, scratch, sizeof(scratch), &peer_len);

	tracepoint(lttng_ust_fi, op_exit, op, h->ep_id, h->src, peer_str,
		   (dir == 'R') ? "rx" : "tx", (uint64_t)iov_count,
		   (uint64_t)bytes, (int64_t)ret,
		   /* An operation the provider refused never became
		    * outstanding, so it will produce no completion and must
		    * not be joined to one. */
		   ret ? NULL : context);
}

void fit_record_cq(struct fit_cq *c, const char *op, fi_addr_t peer,
		   size_t bytes, ssize_t ret, const void *context,
		   uint64_t now)
{
	char scratch[FIT_ADDR_MAX];
	const char *peer_str = "fi_addr_unspec";
	size_t peer_len = sizeof("fi_addr_unspec") - 1;
	const char *src = "unknown";
	struct fit_ep *h = c->ep;

	(void)now;	/* LTTng stamps the event; see fit_now_us() */

	/* The CQ usually serves exactly one endpoint, in which case its local
	 * address and id describe this record too. */
	if (h) {
		if (!h->src_valid)
			fit_ep_resolve_src(h);
		src = h->src;
		peer_str = fit_ep_peer_str(h, peer, scratch, sizeof(scratch),
					   &peer_len);
	}

	tracepoint(lttng_ust_fi, cq_completion, op,
		   h ? h->ep_id : c->cq_id, src, peer_str,
		   (uint64_t)bytes, (int64_t)ret, context);
}

/* ------------------------------------------------------------------ */
/* control-path records                                               */
/* ------------------------------------------------------------------ */

void fit_record_ctrl_entry(const char *op, unsigned int obj_id)
{
	tracepoint(lttng_ust_fi, ctrl_entry, op, obj_id);
}

void fit_record_ctrl_exit(const char *op, unsigned int obj_id, int ret)
{
	tracepoint(lttng_ust_fi, ctrl_exit, op, obj_id, (int64_t)ret);
}

/* ------------------------------------------------------------------ */
/* endpoint description                                               */
/* ------------------------------------------------------------------ */

/*
 * Replaces fit_topo_record(), which appended a row to the sidecar CSV that
 * fitrace-merge later joined against.  Same data, same once-per-endpoint
 * cadence (fit_addr.c guards it with h->topo_written); it is now an event and
 * the analysis side builds the map in memory.
 *
 * The signature gained the ep's source address because the sidecar was keyed
 * by NIC address while the interval stream is keyed by ep_id, and the mapping
 * between them has to come from somewhere.
 */
void fit_record_ep_info(unsigned int ep_id, const char *src, size_t src_len,
			const struct fit_topo *t)
{
	(void)src_len;

	if (!fit_armed || !src)
		return;

	tracepoint(lttng_ust_fi, ep_info, ep_id, src,
		   (t && t->valid) ? t->name : "",
		   (t && t->valid) ? 1u : 0u,
		   (t && t->valid) ? t->group : 0u,
		   (t && t->valid) ? t->sw : 0u,
		   (t && t->valid) ? t->port : 0u);
}

/*
 * No __attribute__((destructor)) flush.  LTTng's own atexit handling drains
 * the ring buffers, and the blocking channel that iprof configures
 * (--blocking-timeout=inf) means nothing is dropped on the way out.
 */
