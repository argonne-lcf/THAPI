/*
 * fi_trace -- NIC address rendering
 *
 * The local address comes from fi_getname() on the endpoint.  A remote
 * peer is identified in the API only by an opaque fi_addr_t, so it is
 * reverse-mapped through the endpoint's address vector with
 * fi_av_lookup() and then formatted with fi_av_straddr() -- which is
 * what produces the "fi_addr_cxi://0x04656600" form.
 *
 * Both are cached: the local address once per endpoint, remote ones in
 * a small open-addressing table keyed by fi_addr_t.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "fi_trace.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIT_PEER_CAP_INIT 64

static void raw_to_hex(const void *addr, size_t len, char *out, size_t out_len) {
  static const char hex[] = "0123456789abcdef";
  const unsigned char *b = addr;
  size_t i, n = 0;

  if (out_len < 4) {
    if (out_len)
      out[0] = '\0';
    return;
  }
  out[n++] = '0';
  out[n++] = 'x';
  for (i = 0; i < len && n + 2 < out_len; i++) {
    out[n++] = hex[b[i] >> 4];
    out[n++] = hex[b[i] & 0xf];
  }
  out[n] = '\0';
}

/* Format a raw provider address, preferring the provider's own
 * stringifier so the output matches fi_info / existing telemetry. */
static void
format_raw(struct fid_av *av, const void *raw, size_t raw_len, char *out, size_t out_len) {
  if (av && av->ops && av->ops->straddr) {
    size_t len = out_len;
    const char *s = fi_av_straddr(av, raw, out, &len);

    if (s) {
      out[out_len - 1] = '\0';
      return;
    }
  }
  raw_to_hex(raw, raw_len, out, out_len);
}

void fit_ep_resolve_src(struct fit_ep *h) {
  char raw[FIT_RAW_ADDR_MAX];
  size_t raw_len = sizeof(raw);
  int ret;

  pthread_mutex_lock(&h->lock);
  if (h->src_valid)
    goto out;

  /* Mark resolved up front: if the provider cannot answer we do not
   * want to retry fi_getname() on every single operation. */
  h->src_valid = 1;
  snprintf(h->src, sizeof(h->src), "unknown");

  if (!fit_cfg.resolve || !h->ep || !h->ep->cm || !h->ep->cm->getname)
    goto out;

  ret = fi_getname(&h->ep->fid, raw, &raw_len);
  if (ret) {
    /* Not enabled yet -- allow a later retry. */
    if (ret == -FI_EOPBADSTATE || ret == -FI_ENOENT)
      h->src_valid = 0;
    goto out;
  }
  if (raw_len > sizeof(raw))
    goto out;

  format_raw(h->av, raw, raw_len, h->src, sizeof(h->src));
out:
  fit_sanitize(h->src, &h->src_len);

  /* The two halves of "where is this endpoint" become available at
   * different moments: the dragonfly coordinates at domain creation,
   * the NIC address only after FI_ENABLE.  This is the first point
   * where both are known, so it is where the pair gets written --
   * once per endpoint, never on the hot path. */
  if (h->src_valid && !h->topo_written) {
    h->topo_written = 1;
    fit_record_ep_info(h->ep_id, h->src, h->src_len, h->topo);
  }

  pthread_mutex_unlock(&h->lock);
}

/* ------------------------------------------------------------------ */
/* dragonfly location                                                  */
/*                                                                     */
/* CXI will say which group, switch and port a NIC is wired to, but    */
/* not through the core API -- it is a provider extension, reached by  */
/* opening a named ops structure on the domain.  The name carries a    */
/* version, and ->topology() does not exist in the oldest ones, so try */
/* newest first and stop at the first that answers.                    */
/* ------------------------------------------------------------------ */

#ifdef HAVE_FI_CXI_TOPOLOGY
#include <rdma/fi_cxi_ext.h>
#include <rdma/fi_ext.h>
#endif

void fit_topo_resolve(struct fit_topo *t, struct fid_domain *domain, const struct fi_info *info) {
  memset(t, 0, sizeof(*t));

  /* The NIC's name ("cxi3") is worth keeping even when the provider
   * will not give coordinates: it is how the node's own tooling and
   * the counter telemetry refer to the device. */
  if (info && info->domain_attr && info->domain_attr->name)
    snprintf(t->name, sizeof(t->name), "%s", info->domain_attr->name);
  else
    snprintf(t->name, sizeof(t->name), "unknown");
  fit_sanitize(t->name, &t->name_len);

#ifdef HAVE_FI_CXI_TOPOLOGY
  {
    static const char *const versions[] = {"dom_ops_v6", "dom_ops_v5", "dom_ops_v4", NULL};
    int i;

    if (!domain || !domain->fid.ops || !domain->fid.ops->ops_open) {
      fit_log("topology %s: domain has no ops_open", t->name);
      return;
    }

    /* Keep the table local to this domain; never reuse another NIC's
     * extension pointer. Try the same versions as bidir_intra.cpp. */
    for (i = 0; versions[i]; i++) {
      struct fi_cxi_dom_ops *ops = NULL;
      unsigned grp = 0, sw = 0, port = 0;
      int ret = fi_open_ops(&domain->fid, versions[i], 0, (void **)&ops, NULL);

      if (ret || !ops || !ops->topology) {
        fit_log("topology %s: %s unavailable (ret=%d)", t->name, versions[i], ret);
        continue;
      }

      ret = ops->topology(&domain->fid, &grp, &sw, &port);
      if (ret) {
        fit_log("topology %s: %s query failed (ret=%d)", t->name, versions[i], ret);
        continue;
      }

      t->group = grp;
      t->sw = sw;
      t->port = port;
      t->valid = 1;
      fit_log("topology %s: %s group=%u switch=%u port=%u", t->name, versions[i], grp, sw, port);
      return;
    }
    fit_log("topology %s: no supported query succeeded", t->name);
  }
#else
  (void)domain;
  fit_log("topology %s: disabled at build time; run make check-topology "
          "with the target libfabric headers, then rebuild",
          t->name);
#endif
}

/* ------------------------------------------------------------------ */
/* the topology sidecar                                                */
/*                                                                     */
/* One row per endpoint, in its own small file next to the traces.     */
/* The alternative -- four more columns on every record -- would repeat*/
/* a constant tens of millions of times to say something that changes  */
/* never.  The merge tool reads the union of these files and joins on  */
/* the NIC address, which is enough to place both ends of every flow:  */
/* a peer this rank only ever sent to is some other rank's local       */
/* endpoint, and that rank wrote the row.                              */
/*                                                                     */
/* Writing is rare -- once per endpoint for the life of the process -- */
/* so it takes a lock and writes straight through, rather than paying  */
/* for the buffered per-thread machinery the hot path needs.           */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* peer cache                                                         */
/* ------------------------------------------------------------------ */

static inline size_t peer_hash(fi_addr_t key, size_t cap) {
  uint64_t h = (uint64_t)key;

  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdull;
  h ^= h >> 33;
  return (size_t)(h & (cap - 1));
}

/* Lock-free lookup.  Entry pointers are stable for the life of the
 * endpoint, so the caller may keep the returned string. */
static const char *peer_find(struct fit_ep *h, fi_addr_t key, size_t *len_out) {
  struct fit_peer_tab *tab;
  size_t i, slot, cap;

  tab = atomic_load_explicit(&h->peers, memory_order_acquire);
  if (!tab)
    return NULL;

  cap = tab->cap;
  slot = peer_hash(key, cap);
  for (i = 0; i < cap; i++) {
    struct fit_peer *e =
        atomic_load_explicit(&tab->slot[(slot + i) & (cap - 1)], memory_order_acquire);

    if (!e)
      return NULL;
    if (e->key == key) {
      *len_out = e->len;
      return e->str;
    }
  }
  return NULL;
}

/* Caller holds h->lock. */
static void tab_place(struct fit_peer_tab *tab, struct fit_peer *e) {
  size_t slot = peer_hash(e->key, tab->cap);
  size_t i;

  for (i = 0; i < tab->cap; i++) {
    _Atomic(struct fit_peer *) *s = &tab->slot[(slot + i) & (tab->cap - 1)];

    if (!atomic_load_explicit(s, memory_order_relaxed)) {
      atomic_store_explicit(s, e, memory_order_release);
      return;
    }
  }
}

/* Caller holds h->lock.  The old table is retired rather than freed so
 * that a concurrent reader still walking it stays safe. */
static int peer_grow(struct fit_ep *h) {
  struct fit_peer_tab *old = atomic_load_explicit(&h->peers, memory_order_relaxed);
  size_t new_cap = old ? old->cap * 2 : FIT_PEER_CAP_INIT;
  struct fit_peer_tab *fresh;
  size_t i;

  fresh = calloc(1, sizeof(*fresh) + new_cap * sizeof(fresh->slot[0]));
  if (!fresh)
    return -1;

  fresh->cap = new_cap;
  fresh->older = old;

  if (old) {
    for (i = 0; i < old->cap; i++) {
      struct fit_peer *e = atomic_load_explicit(&old->slot[i], memory_order_relaxed);

      if (e)
        tab_place(fresh, e);
    }
  }

  atomic_store_explicit(&h->peers, fresh, memory_order_release);
  return 0;
}

/* Caller holds h->lock.  Returns the stable string, or NULL on OOM. */
static const char *peer_insert(struct fit_ep *h, fi_addr_t key, const char *str, size_t *len_out) {
  struct fit_peer_tab *tab = atomic_load_explicit(&h->peers, memory_order_relaxed);
  struct fit_peer *e;

  /* Keep the load factor at or below 1/2 so probing stays short. */
  if (!tab || (h->peer_cnt + 1) * 2 > tab->cap) {
    if (peer_grow(h))
      return NULL;
    tab = atomic_load_explicit(&h->peers, memory_order_relaxed);
  }

  e = calloc(1, sizeof(*e));
  if (!e)
    return NULL;

  e->key = key;
  snprintf(e->str, sizeof(e->str), "%s", str);
  fit_sanitize(e->str, &e->len);

  tab_place(tab, e);
  h->peer_cnt++;
  *len_out = e->len;
  return e->str;
}

void fit_ep_peers_free(struct fit_ep *h) {
  struct fit_peer_tab *tab = atomic_load_explicit(&h->peers, memory_order_relaxed);
  size_t i;

  if (tab) {
    for (i = 0; i < tab->cap; i++)
      free(atomic_load_explicit(&tab->slot[i], memory_order_relaxed));
  }
  while (tab) {
    struct fit_peer_tab *older = tab->older;

    free(tab);
    tab = older;
  }
  atomic_store_explicit(&h->peers, NULL, memory_order_relaxed);
}

/*
 * fi_av_remove() frees an fi_addr_t for reuse, so every cached mapping
 * becomes suspect the moment one happens.  Retire the whole table rather
 * than try to work out which entries died: removals are rare, and the
 * retired table stays reachable through ->older so a concurrent reader
 * walking it is still safe.  Caller holds h->lock.
 */
static void peer_retire_stale(struct fit_ep *h, unsigned int gen) {
  struct fit_peer_tab *old, *fresh;

  if (h->peer_gen == gen)
    return;

  old = atomic_load_explicit(&h->peers, memory_order_relaxed);
  if (old) {
    fresh = calloc(1, sizeof(*fresh) + FIT_PEER_CAP_INIT * sizeof(fresh->slot[0]));
    if (!fresh)
      return; /* keep the stale table over none */
    fresh->cap = FIT_PEER_CAP_INIT;
    fresh->older = old;
    atomic_store_explicit(&h->peers, fresh, memory_order_release);
    fit_log("address vector changed; dropped %u cached peers", (unsigned)h->peer_cnt);
    h->peer_cnt = 0;
  }
  h->peer_gen = gen;
}

const char *fit_ep_peer_str(
    struct fit_ep *h, fi_addr_t peer, char *scratch, size_t scratch_len, size_t *len_out) {
  char raw[FIT_RAW_ADDR_MAX];
  size_t raw_len = sizeof(raw);
  const char *cached;
  unsigned int gen;
  int resolvable;

  if (peer == FI_ADDR_UNSPEC) {
    *len_out = sizeof("fi_addr_unspec") - 1;
    return "fi_addr_unspec";
  }
  if (peer == FI_ADDR_NOTAVAIL) {
    *len_out = sizeof("fi_addr_notavail") - 1;
    return "fi_addr_notavail";
  }

  /* The common case: already seen this peer, and no fi_av_remove()
   * has invalidated the mapping since.  Both resolved and unresolved
   * renderings are cached, so neither path formats a string more than
   * once per peer. */
  gen = atomic_load_explicit(&fit_av_gen, memory_order_acquire);
  if (gen == h->peer_gen) {
    cached = peer_find(h, peer, len_out);
    if (cached)
      return cached;
  }

  resolvable = fit_cfg.resolve && h->av && h->av->ops && h->av->ops->lookup;

  if (resolvable && !fi_av_lookup(h->av, peer, raw, &raw_len) && raw_len <= sizeof(raw))
    format_raw(h->av, raw, raw_len, scratch, scratch_len);
  else
    snprintf(scratch, scratch_len, "fi_addr://0x%016llx", (unsigned long long)peer);

  pthread_mutex_lock(&h->lock);
  peer_retire_stale(h, gen);
  cached = peer_find(h, peer, len_out); /* lost a race? */
  if (!cached)
    cached = peer_insert(h, peer, scratch, len_out);
  pthread_mutex_unlock(&h->lock);

  if (cached)
    return cached;

  fit_sanitize(scratch, len_out);
  return scratch;
}
