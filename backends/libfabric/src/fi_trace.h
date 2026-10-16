/*
 * fi_trace -- LD_PRELOAD per-operation tracer for libfabric
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef FI_TRACE_H
#define FI_TRACE_H

#define _GNU_SOURCE

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>

#define FIT_ADDR_MAX 96 /* "fi_addr_cxi://0x04656600" and friends */
#define FIT_RAW_ADDR_MAX 64
#define FIT_REC_MAX 1024 /* worst-case bytes of one CSV line        */

#ifndef fit_container_of
#define fit_container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

struct fit_ep;
struct fit_cq;
struct fit_topo;

void fit_record_entry(struct fit_ep *h, const char *op, char dir, const void *context);
void fit_record_ep_info(unsigned int ep_id,
                        const char *src,
                        size_t src_len,
                        const struct fit_topo *t);
void fit_record_ctrl_entry(const char *op, unsigned int obj_id);
void fit_record_ctrl_exit(const char *op, unsigned int obj_id, int ret);

/* ------------------------------------------------------------------ */
/* configuration (read once from the environment at load time)        */
/* ------------------------------------------------------------------ */

struct fit_config {
  int enabled;   /* FI_TRACE_ENABLE      */
  int resolve;   /* FI_TRACE_RESOLVE     */
  int trace_msg; /* FI_TRACE_OPS         */
  int trace_tagged;
  int trace_rma;
  int trace_rx;         /* FI_TRACE_RX          */
  int trace_srx;        /* FI_TRACE_SRX         */
  int trace_eagain;     /* FI_TRACE_EAGAIN      */
  int trace_cq;         /* FI_TRACE_CQ          */
  int verbose;          /* FI_TRACE_VERBOSE     */
  size_t bufsz;         /* FI_TRACE_BUFSZ       */
  uint64_t max_records; /* FI_TRACE_MAX_RECORDS */
  char dir[4096];       /* FI_TRACE_DIR         */
  char host[256];
  size_t host_len;
};

/* CSV fields are sanitised and measured once, where they are produced,
 * so the hot path only has to memcpy them. */
void fit_sanitize(char *s, size_t *len_out);

extern struct fit_config fit_cfg;
extern int fit_armed;

void fit_config_init(void);
void fit_log(const char *fmt, ...);

/* ------------------------------------------------------------------ */
/* peer (fi_addr_t -> NIC address string) cache                       */
/*                                                                    */
/* Entries are individually allocated and never moved or freed while  */
/* the endpoint lives, so a lookup can hand back a stable pointer and */
/* the hot path neither copies the string nor takes a lock.  Only     */
/* inserts (one per peer, ever) serialise on the endpoint mutex.      */
/* ------------------------------------------------------------------ */

struct fit_peer {
  fi_addr_t key;
  size_t len; /* strlen(str), precomputed */
  char str[FIT_ADDR_MAX];
};

struct fit_peer_tab {
  size_t cap;                 /* power of two */
  struct fit_peer_tab *older; /* retired tables, freed at ep close */
  _Atomic(struct fit_peer *) slot[];
};

/* ------------------------------------------------------------------ */
/* where a NIC sits in the fabric                                     */
/*                                                                    */
/* A dragonfly address is the part of "which NIC talked to which NIC" */
/* that the NIC address itself cannot tell you: two endpoints whose   */
/* addresses look equally far apart may be on the same switch or      */
/* three hops away.  CXI exposes it through a domain extension, so it */
/* is resolved once per domain and inherited by every endpoint on it. */
/*                                                                    */
/* This is deliberately not a per-record column.  It never changes    */
/* for the life of a domain, so repeating it on millions of rows      */
/* would cost bandwidth and tell you nothing new; it goes in a        */
/* sidecar file keyed by NIC address, and the merge tool joins it.    */
/* ------------------------------------------------------------------ */

struct fit_topo {
  int valid;      /* 0 if the provider would not say */
  unsigned group; /* dragonfly group                 */
  unsigned sw;    /* switch within the group         */
  unsigned port;  /* port on that switch             */
  char name[64];  /* domain name, e.g. "cxi2"        */
  size_t name_len;
};

/* Ask the provider where this domain's NIC is.  Leaves t->valid 0 on
 * anything that is not CXI, or a CXI too old to answer. */
void fit_topo_resolve(struct fit_topo *t, struct fid_domain *domain, const struct fi_info *info);

/* Record one endpoint's location, once, when its address is known. */

/* ------------------------------------------------------------------ */
/* per-object hook state                                              */
/*                                                                    */
/* Each wrapped object gets a private *copy* of the provider vtable   */
/* embedded in the hook struct.  The object's vtable pointer is then  */
/* retargeted at that copy, so a wrapper can recover its own state    */
/* with container_of() -- no hash lookup on the hot path.             */
/* ------------------------------------------------------------------ */

struct fit_ep {
  /* our vtables -- must stay first-class members for container_of */
  struct fi_ops_msg msg;
  struct fi_ops_tagged tagged;
  struct fi_ops_rma rma;
  struct fi_ops fid_ops;
  struct fi_ops_ep ep_ops;

  /* the provider's originals */
  const struct fi_ops_msg *o_msg;
  const struct fi_ops_tagged *o_tagged;
  const struct fi_ops_rma *o_rma;
  const struct fi_ops *o_fid;
  const struct fi_ops_ep *o_ep;

  struct fid_ep *ep;
  struct fid_av *av; /* bound AV, used to render addresses */
  unsigned int ep_id;

  /* Points into the owning domain's hook state, which outlives every
   * endpoint on it.  NULL for an endpoint we met without its domain. */
  const struct fit_topo *topo;
  int topo_written;

  char src[FIT_ADDR_MAX];
  size_t src_len;
  int src_valid;

  pthread_mutex_t lock; /* serialises inserts + src resolution */
  _Atomic(struct fit_peer_tab *) peers;
  size_t peer_cnt;       /* written under lock */
  unsigned int peer_gen; /* fit_av_gen when the table was built */
};

/*
 * An fi_addr_t is an index into the address vector, and fi_av_remove()
 * frees that index for reuse.  A cached fi_addr_t -> NIC address mapping
 * is therefore only valid until the next removal.  Removals are rare
 * (most applications never call it), so rather than track which entries
 * died, bump a global counter and let each endpoint retire its whole
 * table the next time it looks something up.
 */
extern _Atomic unsigned int fit_av_gen;

void fit_ep_peers_free(struct fit_ep *h);

struct fit_domain {
  struct fi_ops_domain ops;
  struct fi_ops fid_ops;
  const struct fi_ops_domain *orig;
  const struct fi_ops *o_fid;

  /* Resolved once at hook time and handed to every endpoint this
   * domain creates.  The address of this member is what endpoints
   * hold, so the struct must not move. */
  struct fit_topo topo;
};

struct fit_fabric {
  struct fi_ops_fabric ops;
  struct fi_ops fid_ops;
  const struct fi_ops_fabric *orig;
  const struct fi_ops *o_fid;
};

struct fit_av {
  struct fi_ops_av ops;
  struct fi_ops fid_ops;
  const struct fi_ops_av *orig;
  const struct fi_ops *o_fid;
};

/*
 * Completion queue.  Hooking this is what turns a log of *submitted*
 * operations into a log of transfers with a measurable duration: the
 * posting record and the completion record carry the same op_context, so
 * subtracting their timestamps gives the latency the application saw.
 */
struct fit_cq {
  struct fi_ops_cq ops;
  struct fi_ops fid_ops;
  const struct fi_ops_cq *orig;
  const struct fi_ops *o_fid;

  struct fid_cq *cq;
  enum fi_cq_format format;
  size_t entry_sz; /* 0 if the format is unknown */

  /* The endpoint this CQ was bound to, so a completion can be
   * attributed to a local NIC address.  Left NULL if more than one
   * endpoint binds the same CQ, because then it would be a guess. */
  struct fit_ep *ep;
  int ep_ambiguous;
  unsigned int cq_id;
};

/* ------------------------------------------------------------------ */
/* writer                                                             */
/* ------------------------------------------------------------------ */

// void fit_writer_init(void);
void fit_writer_flush_all(void);

/* Emit one CSV record.  dir is 'T' (local initiates) or 'R' (local posts
 * a receive).  peer is the fi_addr_t supplied by the caller; it is
 * rendered through the endpoint's AV when possible.  context is the
 * application's op_context, recorded so the completion can be matched to
 * this record; it is dropped when the provider refused the operation,
 * since nothing became outstanding. */
void fit_record(struct fit_ep *h,
                const char *op,
                char dir,
                fi_addr_t peer,
                size_t bytes,
                size_t iov_count,
                ssize_t ret,
                const void *context);

/* Emit one completion record.  now is the time the batch was reaped. */
void fit_record_cq(struct fit_cq *c,
                   const char *op,
                   fi_addr_t peer,
                   size_t bytes,
                   ssize_t ret,
                   const void *context,
                   uint64_t now);

uint64_t fit_now_us(void);

/* ------------------------------------------------------------------ */
/* address rendering                                                  */
/* ------------------------------------------------------------------ */

void fit_ep_resolve_src(struct fit_ep *h);

/* Returns a sanitised, NUL-terminated peer string and its length.  The
 * result may point into the endpoint's peer cache (stable until the
 * endpoint closes) or into the caller's scratch buffer. */
const char *fit_ep_peer_str(
    struct fit_ep *h, fi_addr_t peer, char *scratch, size_t scratch_len, size_t *len_out);

/* ------------------------------------------------------------------ */
/* installers                                                         */
/* ------------------------------------------------------------------ */

void fit_hook_fabric(struct fid_fabric *fabric);
void fit_hook_domain(struct fid_domain *domain, const struct fi_info *info);
/* topo may be NULL; it is borrowed from the owning domain and outlives
 * the endpoint. */
void fit_hook_ep(struct fid_ep *ep, const struct fit_topo *topo);
void fit_hook_av(struct fid_av *av);
void fit_hook_cq(struct fid_cq *cq, enum fi_cq_format format);

static inline size_t fit_iov_len(const struct iovec *iov, size_t count) {
  size_t i, total = 0;

  if (!iov)
    return 0;
  for (i = 0; i < count; i++)
    total += iov[i].iov_len;
  return total;
}

static inline size_t fit_rma_iov_len(const struct fi_rma_iov *iov, size_t count) {
  size_t i, total = 0;

  if (!iov)
    return 0;
  for (i = 0; i < count; i++)
    total += iov[i].len;
  return total;
}

#endif /* FI_TRACE_H */
