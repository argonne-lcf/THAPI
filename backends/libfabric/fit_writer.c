/*
 * fi_trace -- record writer
 *
 * One CSV file per thread, so records never interleave and the only
 * lock on the hot path is the thread's own -- uncontended except when a
 * shutdown path flushes every buffer.  Buffers are flushed on overflow,
 * at thread exit, and from a library destructor.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "fi_trace.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct fit_config fit_cfg;
int fit_armed;
_Atomic unsigned int fit_av_gen;

static const char fit_csv_header[] =
	"timestamp_us,pid,tid,ep_id,src_hostname,src_addr,dest_addr,"
	"transfer_func,direction,iov_count,size_bytes,ret,op_context\n";

/* ------------------------------------------------------------------ */
/* logging                                                            */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* configuration                                                      */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* per-thread buffers                                                 */
/* ------------------------------------------------------------------ */

/*
 * Each buffer belongs to one thread, but it is not private to it:
 * fit_writer_flush_all() is called from fi_close() and from the library
 * destructor, and flushes *every* thread's buffer -- possibly while the
 * owning thread is in the middle of appending to it.  Without
 * synchronisation that races on both ->len and the bytes themselves, and
 * produces truncated or interleaved records.
 *
 * So each buffer carries its own mutex.  It is uncontended in the normal
 * case (the owning thread is the only one touching it), which on glibc is
 * a single atomic compare-exchange, and contended only during the rare
 * foreign flush.  Holding it across the write() is deliberate: a foreign
 * flusher blocks the owner for the duration of one write syscall at
 * shutdown, which is much cheaper than double-buffering.
 */
struct fit_tls {
	pthread_mutex_t  lock;
	int              fd;
	char            *buf;
	size_t           cap;
	size_t           len;
	uint64_t         nrec;
	int              broken;
	pid_t            tid;
	struct fit_tls  *next;		/* global list, for destructor flush */
};

/* glibc no longer caches getpid(), and a syscall per record dominates
 * the hot path, so cache it here and refresh it across fork(). */
static pid_t fit_pid;

static void fit_pid_refresh(void)
{
	fit_pid = getpid();
}

static pthread_key_t   fit_key;
static pthread_once_t  fit_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t fit_list_lock = PTHREAD_MUTEX_INITIALIZER;
static struct fit_tls *fit_list;

/* Caller holds t->lock. */
static void tls_flush_locked(struct fit_tls *t)
{
	size_t off = 0;

	if (t->broken || t->fd < 0 || !t->len)
		goto done;

	while (off < t->len) {
		ssize_t n = write(t->fd, t->buf + off, t->len - off);

		if (n > 0) {
			off += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		fit_log("write failed: %s", strerror(errno));
		t->broken = 1;
		break;
	}
done:
	t->len = 0;
}

static void tls_flush(struct fit_tls *t)
{
	pthread_mutex_lock(&t->lock);
	tls_flush_locked(t);
	pthread_mutex_unlock(&t->lock);
}

static void tls_destroy(void *arg)
{
	struct fit_tls *t = arg, **pp;

	if (!t)
		return;

	tls_flush(t);

	pthread_mutex_lock(&fit_list_lock);
	for (pp = &fit_list; *pp; pp = &(*pp)->next) {
		if (*pp == t) {
			*pp = t->next;
			break;
		}
	}
	pthread_mutex_unlock(&fit_list_lock);

	/* Safe to tear down now: a concurrent fit_writer_flush_all() holds
	 * the list lock for its whole iteration, so it either finished
	 * before the unlink above or can no longer reach this buffer. */
	if (t->fd >= 0)
		close(t->fd);
	pthread_mutex_destroy(&t->lock);
	free(t->buf);
	free(t);
}

static void key_create(void)
{
	pthread_key_create(&fit_key, tls_destroy);
}

static struct fit_tls *tls_get(void)
{
	struct fit_tls *t;
	char path[sizeof(fit_cfg.dir) + sizeof(fit_cfg.host) + 64];
	pid_t tid;

	pthread_once(&fit_once, key_create);

	t = pthread_getspecific(fit_key);
	if (t)
		return t;

	t = calloc(1, sizeof(*t));
	if (!t)
		return NULL;

	pthread_mutex_init(&t->lock, NULL);
	tid = (pid_t)syscall(SYS_gettid);
	t->tid = tid;
	t->fd  = -1;
	t->cap = fit_cfg.bufsz;
	t->buf = malloc(t->cap);
	if (!t->buf) {
		pthread_mutex_destroy(&t->lock);
		free(t);
		return NULL;
	}

	/* Set this before any work that could recurse back in. */
	pthread_setspecific(fit_key, t);

	snprintf(path, sizeof(path), "%s/fitrace.%s.%d.%d.csv", fit_cfg.dir,
		 fit_cfg.host, (int)getpid(), (int)tid);

	t->fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	if (t->fd < 0) {
		fit_log("cannot open %s: %s", path, strerror(errno));
		t->broken = 1;
	} else {
		memcpy(t->buf, fit_csv_header, sizeof(fit_csv_header) - 1);
		t->len = sizeof(fit_csv_header) - 1;
		fit_log("tracing thread %d to %s", (int)tid, path);
	}

	pthread_mutex_lock(&fit_list_lock);
	t->next  = fit_list;
	fit_list = t;
	pthread_mutex_unlock(&fit_list_lock);

	return t;
}

void fit_writer_flush_all(void)
{
	struct fit_tls *t;

	pthread_mutex_lock(&fit_list_lock);
	for (t = fit_list; t; t = t->next)
		tls_flush(t);
	pthread_mutex_unlock(&fit_list_lock);
}

void fit_writer_init(void)
{
	struct stat st;

	fit_pid_refresh();
	pthread_atfork(NULL, NULL, fit_pid_refresh);

	if (stat(fit_cfg.dir, &st) != 0 && mkdir(fit_cfg.dir, 0755) != 0 &&
	    errno != EEXIST)
		fit_log("cannot create %s: %s", fit_cfg.dir, strerror(errno));
}

/* ------------------------------------------------------------------ */
/* fast field appenders                                               */
/* ------------------------------------------------------------------ */

static inline char *put_u64(char *p, uint64_t v)
{
	char tmp[20];
	int n = 0;

	do {
		tmp[n++] = (char)('0' + (v % 10));
		v /= 10;
	} while (v);
	while (n--)
		*p++ = tmp[n];
	return p;
}

static inline char *put_i64(char *p, int64_t v)
{
	if (v < 0) {
		*p++ = '-';
		return put_u64(p, (uint64_t)(-(v + 1)) + 1);
	}
	return put_u64(p, (uint64_t)v);
}

/* Replace anything that could break CSV framing, in place, and report
 * the length.  Called once per distinct string, not per record. */
void fit_sanitize(char *s, size_t *len_out)
{
	size_t i;

	for (i = 0; s[i]; i++) {
		char c = s[i];

		if (c == ',' || c == '\n' || c == '\r' || c == '"')
			s[i] = '_';
	}
	*len_out = i;
}

/* Hot path: the string was sanitised when it was produced. */
static inline char *put_mem(char *p, const char *s, size_t len)
{
	memcpy(p, s, len);
	return p + len;
}

/* Only used for the short, compile-time-constant op names. */
static inline char *put_lit(char *p, const char *s)
{
	while (*s)
		*p++ = *s++;
	return p;
}

/* The op_context column.  It is only ever compared for equality -- it
 * joins a posting record to its completion -- so the raw pointer value is
 * all that is needed, and hex keeps it short. */
static inline char *put_ptr(char *p, const void *v)
{
	static const char hex[] = "0123456789abcdef";
	uintptr_t u = (uintptr_t)v;
	int shift;

	if (!u) {
		*p++ = '0';
		return p;
	}

	*p++ = '0';
	*p++ = 'x';
	for (shift = (int)(sizeof(u) * 8 - 4); shift >= 0; shift -= 4) {
		unsigned d = (unsigned)((u >> shift) & 0xf);

		if (d || p[-1] != 'x' || shift == 0)
			*p++ = hex[d];
	}
	return p;
}

uint64_t fit_now_us(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

/* ------------------------------------------------------------------ */
/* the hot path                                                       */
/* ------------------------------------------------------------------ */

/*
 * Append one record.  Everything that needs resolving has already been
 * resolved by the caller, so all this does is format and memcpy under the
 * buffer lock.
 */
static void fit_emit(unsigned int obj_id, const char *src, size_t src_len,
		     const char *peer, size_t peer_len, const char *op,
		     const char *dir, size_t iov_count, size_t bytes,
		     ssize_t ret, const void *context, uint64_t now)
{
	struct fit_tls *t;
	char *p;

	t = tls_get();
	if (!t || t->broken)
		return;

	pthread_mutex_lock(&t->lock);

	if (fit_cfg.max_records && t->nrec >= fit_cfg.max_records)
		goto out;

	if (t->len + FIT_REC_MAX > t->cap)
		tls_flush_locked(t);
	if (t->broken)
		goto out;

	p = t->buf + t->len;

	p = put_u64(p, now);                               *p++ = ',';
	p = put_u64(p, (uint64_t)fit_pid);                 *p++ = ',';
	p = put_u64(p, (uint64_t)t->tid);                  *p++ = ',';
	p = put_u64(p, obj_id);                            *p++ = ',';
	p = put_mem(p, fit_cfg.host, fit_cfg.host_len);    *p++ = ',';
	p = put_mem(p, src, src_len);                      *p++ = ',';
	p = put_mem(p, peer, peer_len);                    *p++ = ',';
	p = put_lit(p, op);                                *p++ = ',';
	p = put_lit(p, dir);                               *p++ = ',';
	p = put_u64(p, iov_count);                         *p++ = ',';
	p = put_u64(p, bytes);                             *p++ = ',';
	p = put_i64(p, (int64_t)ret);                      *p++ = ',';
	p = put_ptr(p, context);                           *p++ = '\n';

	t->len = (size_t)(p - t->buf);
	t->nrec++;
out:
	pthread_mutex_unlock(&t->lock);
}

void fit_record(struct fit_ep *h, const char *op, char dir, fi_addr_t peer,
		size_t bytes, size_t iov_count, ssize_t ret,
		const void *context)
{
	char scratch[FIT_ADDR_MAX];
	const char *peer_str;
	size_t peer_len = 0;
	uint64_t now;

	if (dir == 'R' && !fit_cfg.trace_rx)
		return;

	/* A -FI_EAGAIN return means the provider refused the operation and
	 * the application will retry; no bytes moved.  Applications that
	 * spin on a busy endpoint can generate thousands of these, which
	 * swamps the real transfers, so they can be dropped. */
	if (ret == -FI_EAGAIN && !fit_cfg.trace_eagain)
		return;

	now = fit_now_us();

	if (!h->src_valid)
		fit_ep_resolve_src(h);

	peer_str = fit_ep_peer_str(h, peer, scratch, sizeof(scratch),
				   &peer_len);

	/* An operation the provider refused never became outstanding, so it
	 * will produce no completion and must not be joined to one. */
	fit_emit(h->ep_id, h->src, h->src_len, peer_str, peer_len, op,
		 (dir == 'R') ? "rx" : "tx", iov_count, bytes, ret,
		 ret ? NULL : context, now);
}

/*
 * A completion record.  Unlike a posting record this is observed *after*
 * the transfer, so size_bytes is the length the NIC actually moved and,
 * when the application used fi_cq_readfrom(), peer is the real sender
 * rather than the wildcard the receive was posted with.
 *
 * now is passed in because one fi_cq_read() can return a burst of
 * completions, and timestamping them individually would imply a
 * resolution the batch does not have.
 */
void fit_record_cq(struct fit_cq *c, const char *op, fi_addr_t peer,
		   size_t bytes, ssize_t ret, const void *context,
		   uint64_t now)
{
	char scratch[FIT_ADDR_MAX];
	const char *peer_str = "fi_addr_unspec";
	size_t peer_len = sizeof("fi_addr_unspec") - 1;
	const char *src = "unknown";
	size_t src_len = sizeof("unknown") - 1;
	struct fit_ep *h = c->ep;

	/* The CQ usually serves exactly one endpoint, in which case its
	 * local address and id describe this record too. */
	if (h) {
		if (!h->src_valid)
			fit_ep_resolve_src(h);
		src     = h->src;
		src_len = h->src_len;
		peer_str = fit_ep_peer_str(h, peer, scratch, sizeof(scratch),
					   &peer_len);
	}

	fit_emit(h ? h->ep_id : c->cq_id, src, src_len, peer_str, peer_len,
		 op, "cq", 1, bytes, ret, context, now);
}

__attribute__((destructor))
static void fit_fini(void)
{
	fit_writer_flush_all();
}
