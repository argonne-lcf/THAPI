/* Timestamp capture (one uniform scheme for ALL command lists)
 * ============================================================
 *
 *     Kernel/Copy(sig=inj) ──> re-expose user_signal  [on user cl]
 *     drain: zeEventQueryKernelTimestamp(inj)         [host read of OUR event]
 *
 * `inj` is a tracer-owned KERNEL_TIMESTAMP event. The append prologue swaps the
 * user's signal for inj, so the kernel (or copy) signals inj directly and inj
 * carries the real kernel timing; at drain we read it on the HOST via
 * zeEventQueryKernelTimestamp(inj). This is a host-side event read, so it works
 * for any engine (compute or copy) and appends nothing to the device.
 *
 * Re-exposing the user's signal (so their Sync(user_signal) still fires; skipped
 * when they passed none): in-order cls use zeCommandListAppendSignalEvent (the
 * cl runs sequentially, so a signal after the kernel already orders correctly);
 * out-of-order cls use zeCommandListAppendBarrier(wait=inj) to force it after
 * the kernel.
 *
 * INVARIANT: we only ever sync / query / reset OUR OWN inj event. The user's
 * event is read-only to us: we stash timing under it (so the user's own
 * zeEventQueryKernelTimestamp returns kernel timing) but never sync or reset
 * it. The one place we sync a user object is the Execute epilogue's
 * zeCommandQueueSynchronize — a QUEUE, never an event — to observe completion
 * of work the user already submitted.
 *
 * Algorithm
 * ---------
 * On profiled Append (cl, sig=user_sig, waits=user_waits):
 *   - allocate inj from per-context pool; swap user_sig -> inj
 *   - Barrier(wait=inj, sig=user_sig) on the user cl (skip if user_sig NULL)
 *   - allocate a slot {inj, attr=user_sig, waits=copy(user_waits)}; instantiate
 *
 * instantiate(s):
 *   - s.preds = [event_latest_signaled[w] for w in s.waits if live]
 *                + previous live slot in same cl (if cl is in-order)
 *   - s.live = true; event_latest_signaled[s.attr] = &s
 *
 * On Execute(q, cl) prologue — one critical section, BEFORE L0 submit
 * (concurrent Executes/Syncs observe in_flight_q atomically):
 *   - if cl.in_flight_q: Synchronize(in_flight_q); drain_cl(cl). Drains the
 *     prior instance (same or another thread's queue), host-reading its timing
 *     from inj and resetting inj, BEFORE this submission re-signals inj.
 *   - re-instantiate every not-yet-live slot (the inj-signal + user-signal chain
 *     are baked in the closed body and re-fire automatically on replay).
 *   - cl.in_flight_q = q; index cl under q (and its fence) for sync lookup.
 *
 * On Sync (the synced anchor tells us what to drain):
 *   - Sync(ev):  drain(event_latest_signaled[ev])
 *   - Sync(q):   drain_cl(cl) for every cl in the q-index bucket for q
 *   - Sync(cl):  drain_cl(cl)
 *
 * drain(s):
 *   - for p in s.preds: drain(p)
 *   - HostSynchronize(inj) (cheap guard), zeEventQueryKernelTimestamp(inj) -> r
 *   - stash r under s.attr, emit tracepoint(s.attr or inj)
 *   - reset our inj (regular cls; immediate cls reset it on pool recycle) so it
 *     starts clean for its next Execute. We only ever reset OUR event.
 *   - clear event_latest_signaled[s.attr]; clear s.live and s.preds
 *
 * Concurrency
 * -----------
 * One global mutex (_ze_state_mutex) covers all tracer state: the cl registry,
 * every cl's chunks/slots/preds, the event freelist + pool registry, the
 * latest-signaled map. Append / Execute / Drain / Destroy all take it.
 *
 * Per-cl mutexes don't work because drain follows cross-cl pred edges
 * (event_latest_signaled[ev] can point at a slot in any cl) and mutates the
 * pred's chunk via _slot_release. One global mutex sidesteps the cross-cl
 * ordering problem entirely.
 *
 * Perf: the L0 call inside the critical section (AppendBarrier) just queues
 * work on the GPU asynchronously, so the held region is short. Drain is
 * host-blocking (zeEventHostSynchronize on our inj) and was serial anyway.
 */

/* Always-on tracer log. Prefixes THAPI(func:line) so messages are
 * grep-able across the bench/test harness which often interleaves
 * tracer and user output. GCC's `, ##__VA_ARGS__` extension swallows
 * the leading comma when the variadic list is empty. fflush so the
 * line lands even if we abort() right after. */
#define _THAPI_LOG(fmt, ...)                                                                       \
  do {                                                                                             \
    fprintf(stderr, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                 \
    fflush(stderr);                                                                                \
  } while (0)

#ifdef THAPI_DEBUG
#define THAPI_DBGLOG(fmt, ...) _THAPI_LOG(fmt, ##__VA_ARGS__)
#else
#define THAPI_DBGLOG(...)                                                                          \
  do {                                                                                             \
  } while (0)
#endif

/* Tracer invariant check: print + abort. Unconditional (not gated on
 * NDEBUG) — silently dropping the check would let the bug ship bad
 * data instead of crashing. Use for "this can never happen" preconditions
 * inside the tracer, not for user-input validation. */
#define _THAPI_ASSERT(cond, fmt, ...)                                                              \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      _THAPI_LOG("assertion failed: %s — " fmt, #cond, ##__VA_ARGS__);                             \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)

/* Wrap a tracer-issued L0 call whose failure means we'd either hang the
 * user (sync chain Barrier) or produce a non-self-consistent trace
 * (QKT, event create, ...). Defensive: print + abort so the bug surfaces
 * under sanitizers/CI rather than ship bad data. NOT for driver query
 * calls (Get*Handle, GetCommandQueueGroupProperties) — those can fail
 * transiently during teardown and have graceful fallbacks. */
#define _ZE_MUST(call)                                                                             \
  do {                                                                                             \
    ze_result_t _r = (call);                                                                       \
    _THAPI_ASSERT(_r == ZE_RESULT_SUCCESS, "%s = 0x%x", #call, _r);                                \
  } while (0)

static int _do_profile = 0;
static int _do_chained_structs = 0;
static int _do_paranoid_drift = 0;
static int _do_paranoid_memory_location = 0;
static int _do_ddi_table_forward = 0;

pthread_mutex_t ze_closures_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ze_closure {
  void *ptr;
  void *c_ptr;
  UT_hash_handle hh;
  ffi_cif cif;
  ffi_closure *closure;
  ffi_type **types;
};

struct ze_closure *ze_closures = NULL;

struct _ze_event_h;
struct _ze_slot;
struct _ze_slot_chunk;

/* Dependency-tracking slot: one per profiled Append. Slots carry the
 * happens-before edges the user established (via cl in-order semantics
 * and via phWaitEvents). At sync time we walk these edges from the
 * synced anchor and drain everything reachable. Drain is pop semantics:
 * after emit, the slot is dropped from the cl's list. */
struct _ze_slot {
  struct _ze_command_list_obj_data *owner; /* cl_data this slot lives in */
  struct _ze_slot_chunk *chunk; /* chunk this slot lives in */
  /* Tracer-owned KERNEL_TIMESTAMP event the kernel/copy signals (the prologue
   * swapped it in for the user's signal). Carries the real kernel timing; drain
   * host-reads it via zeEventQueryKernelTimestamp. Ours — we own its lifecycle
   * (sync/query/reset/dispose); we never touch the user's event. */
  struct _ze_event_h *inj;
  ze_event_handle_t attr; /* user's original signal event (NULL => inj->event) */
  /* User wait events copied at Append time (stable across rebuilds);
   * preds[] is computed at instantiate from waits[] by looking up
   * event_latest_signaled[w] for each w. */
  ze_event_handle_t *waits;
  uint32_t n_waits;
  struct _ze_slot **preds; /* points at slots whose drain must come first (may be in another cl) */
  uint32_t n_preds;
  bool live; /* in-flight (instantiated, not drained) */
  /* Incoming pred edges: count of downstream slots whose preds[] points
   * here AND that have not yet been drained. Incremented at downstream
   * _slot_instantiate, decremented at downstream _slot_drain. Slot is
   * reclaimable iff live==0 AND refs==0. */
  uint32_t refs;
};

#define _ZE_CHUNK_SLOTS 64

/* Slot storage in fixed-size chunks; cl_data->chunks is a utlist DL of
 * these. Imm cls allocate new chunks as needed (no cap); regular cls stop
 * at one chunk — the inj-signal swap and the user-signal barrier are baked
 * into the closed cl body per Append, so adding a chunk after Close would
 * create slots the body doesn't address.
 *
 * The chunk frees itself when n_held drops to 0 AND it is not the tail
 * (new Appends still want to land on the tail). */
struct _ze_slot_chunk {
  uint32_t n_used;              /* slots ever assigned in this chunk (monotonic until chunk free) */
  uint32_t n_held;              /* unreleased slots (n_used minus _slot_release calls) */
  /* Nonzero only on a DETACHED chunk: one whose owning cl was torn down
   * (reset/destroy) while >=1 slot was still referenced as a pred by a live
   * slot in ANOTHER cl. The chunk is removed from cl_data->chunks, its slots'
   * resources are already released and owner==NULL — only the struct survives
   * so the referrers' preds[] pointers stay valid. n_pinned counts those
   * surviving referenced slots; the downstream drain that drops the last ref
   * frees the struct. 0 for normal attached chunks. */
  uint32_t n_pinned;
  struct _ze_slot_chunk *next, *prev;
  struct _ze_slot slots[_ZE_CHUNK_SLOTS];
};

struct _ze_command_list_obj_data {
  void *ptr;
  UT_hash_handle hh;

  struct _ze_slot_chunk *chunks; /* utlist DL_ head; tail = chunks->prev (circular) */

  /* in_flight_q is the queue this cl was last Executed on AND not yet
   * drained. NULL means "not in flight" — safe to Execute without a
   * force-sync. Set on Execute, cleared on drain.
   *
   * Held only for regular cls; immediate cls never Execute. */
  ze_command_queue_handle_t in_flight_q;
  /* The fence (if any) passed to that same Execute. NULL when the user
   * Executed without a fence. Lets a fence-only sync find which cls to
   * drain — the fence signals when all cls in its Execute complete, so
   * zeFenceHostSynchronize(f) drains every cl whose in_flight_fence == f.
   * Set on Execute alongside in_flight_q, cleared together on drain. */
  ze_fence_handle_t in_flight_fence;
  bool is_immediate;
  bool is_in_order;
  /* Count of slots instantiated into the dep graph but not yet drained. Bumped
   * in _slot_instantiate, dropped in _slot_drain — an O(1) "is anything still
   * in flight?" for the sync path. */
  uint32_t n_live;

  /* Cached on first use: context handle for this cl. Immutable for the
   * cl's lifetime. Load-bearing for _on_destroy_context's sweep: lets it
   * associate cls back to their ctx without an L0 roundtrip per cl. */
  ze_context_handle_t cached_context;

  /* Membership in the per-queue / per-fence in-flight indexes (see
   * _ze_q_index / _ze_fence_index below). A cl in flight is linked into both
   * its queue's bucket (q_prev/q_next) and, if Executed with a fence, its
   * fence's bucket (f_prev/f_next), so a queue/fence sync drains exactly the
   * matching cls without scanning every live cl. Linked at Execute, unlinked
   * at drain, both via _cl_index_clear. */
  struct _ze_command_list_obj_data *q_prev, *q_next;
  struct _ze_command_list_obj_data *f_prev, *f_next;
};

struct _ze_command_list_obj_data *_ze_cls = NULL;

/* The single mutex covering all tracer state — see the "Concurrency"
 * section in the file header for rationale. Every static helper in this
 * file that touches tracer state assumes the caller holds it. */
pthread_mutex_t _ze_state_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Pure HASH wrappers. */
static struct _ze_command_list_obj_data *_cl_find(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl = NULL;
  HASH_FIND_PTR(_ze_cls, &command_list, cl);
  return cl;
}

static void _cl_add(struct _ze_command_list_obj_data *cl) { HASH_ADD_PTR(_ze_cls, ptr, cl); }

static struct _ze_command_list_obj_data *_cl_find_and_del(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl = _cl_find(command_list);
  if (cl)
    HASH_DEL(_ze_cls, cl);
  return cl;
}

/* In-flight indexes: queue handle -> the cls currently in flight on that queue,
 * and fence handle -> the cls in flight under that fence. A queue/fence sync
 * completes exactly the cls of the matching Execute, so these let _on_sync
 * drain just those cls instead of scanning every live cl (which is O(live cls)
 * per sync — see bench/sync_scaling). Buckets are created lazily at Execute and
 * freed when they go empty at drain. */
struct _ze_inflight_bucket {
  void *key;                             /* ze_command_queue_handle_t or ze_fence_handle_t */
  struct _ze_command_list_obj_data *cls; /* DL via q_prev/q_next or f_prev/f_next */
  UT_hash_handle hh;
};
static struct _ze_inflight_bucket *_ze_q_index = NULL;
static struct _ze_inflight_bucket *_ze_fence_index = NULL;

static void _index_link(struct _ze_inflight_bucket **index,
                        void *key,
                        struct _ze_command_list_obj_data *cl,
                        int is_fence) {
  if (!key)
    return;
  struct _ze_inflight_bucket *b = NULL;
  HASH_FIND_PTR(*index, &key, b);
  if (!b) {
    b = (struct _ze_inflight_bucket *)calloc(1, sizeof(*b));
    if (!b)
      return;
    b->key = key;
    HASH_ADD_PTR(*index, key, b);
  }
  if (is_fence)
    DL_APPEND2(b->cls, cl, f_prev, f_next);
  else
    DL_APPEND2(b->cls, cl, q_prev, q_next);
}

static void _index_unlink(struct _ze_inflight_bucket **index,
                          void *key,
                          struct _ze_command_list_obj_data *cl,
                          int is_fence) {
  if (!key)
    return;
  struct _ze_inflight_bucket *b = NULL;
  HASH_FIND_PTR(*index, &key, b);
  if (!b)
    return;
  if (is_fence)
    DL_DELETE2(b->cls, cl, f_prev, f_next);
  else
    DL_DELETE2(b->cls, cl, q_prev, q_next);
  if (!b->cls) {
    HASH_DEL(*index, b);
    free(b);
  }
}

/* Link cl into the queue (and, if non-NULL, fence) in-flight indexes. Called
 * once per Execute, after in_flight_q/in_flight_fence are stamped. */
static void _cl_index_set(struct _ze_command_list_obj_data *cl,
                          ze_command_queue_handle_t q,
                          ze_fence_handle_t f) {
  _index_link(&_ze_q_index, q, cl, /*is_fence=*/0);
  _index_link(&_ze_fence_index, f, cl, /*is_fence=*/1);
}

/* Remove cl from both in-flight indexes. Uses cl's own in_flight_q/_fence as
 * the keys, so it MUST run before those are cleared. Idempotent: a cl not in
 * flight has NULL keys and is a no-op. */
static void _cl_index_clear(struct _ze_command_list_obj_data *cl) {
  _index_unlink(&_ze_q_index, cl->in_flight_q, cl, /*is_fence=*/0);
  _index_unlink(&_ze_fence_index, cl->in_flight_fence, cl, /*is_fence=*/1);
}

static inline void _on_create_command_list(ze_command_list_handle_t command_list,
                                           bool immediate,
                                           bool in_order) {
  struct _ze_command_list_obj_data *cl_data =
      (struct _ze_command_list_obj_data *)calloc(1, sizeof(*cl_data));
  if (!cl_data) {
    THAPI_DBGLOG("Failed to allocate memory");
    return;
  }
  cl_data->ptr = (void *)command_list;
  cl_data->is_immediate = immediate;
  cl_data->is_in_order = in_order;

  pthread_mutex_lock(&_ze_state_mutex);
  if (_cl_find(command_list)) {
    pthread_mutex_unlock(&_ze_state_mutex);
    THAPI_DBGLOG("Command list already registered: %p", command_list);
    free(cl_data);
    return;
  }
  _cl_add(cl_data);
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* Wrapper around an injected event we own. Lives either in the per-context
 * free pool (between uses) or anchored to one of cl_data->slots[] (in flight). */
struct _ze_event_h {
  ze_event_handle_t event;
  ze_event_pool_handle_t event_pool;
  ze_context_handle_t context;
  /* doubly-linked list pointers used by the per-context free pool */
  struct _ze_event_h *next, *prev;
};

struct _ze_event_pool_entry {
  ze_context_handle_t context;
  UT_hash_handle hh;
  struct _ze_event_h *events;
};

struct _ze_event_pool_entry *_ze_event_pools = NULL;

/* Per-event tracer state, keyed by the user's event handle. Two facts live
 * here, both populated around drain and both bound to the event's lifetime, so
 * they share one uthash entry (one lookup, one alloc, one eviction):
 *
 *   latest  -> the most recent slot whose attr==ev. Resolves happens-before
 *              edges: when a new Append waits on ev, that slot becomes a pred.
 *              Set at instantiate; cleared at drain/dispose only if it still
 *              points at the draining slot (a newer Append may have overwritten
 *              it — don't clobber that).
 *   kts     -> last kernel-timestamp result we drained for ev. The Append
 *              prologue swaps the user's signal for our inj, so the user's event
 *              carries the barrier/signal op timing, not the kernel's. At drain
 *              we host-read the real kernel result from inj and stash it here so
 *              the user's own zeEventQueryKernelTimestamp is served kernel
 *              timing; re-signaling overwrites.
 *
 * The whole entry is evicted by _on_destroy_event so a recycled handle address
 * (the L0 driver reuses freed event addresses) never serves a dead event's
 * latest slot (a dangling pred -> UAF) or stale kts. The value stays inline in
 * the entry — no per-set heap box. */
struct _ze_event_state_entry {
  ze_event_handle_t ev; /* key */
  struct _ze_slot *latest;
  ze_kernel_timestamp_result_t kts;
  bool has_kts;
  UT_hash_handle hh;
};
static struct _ze_event_state_entry *_ze_event_state = NULL;

/* Find-or-create the entry for ev. NULL only on ev==NULL or OOM. */
static struct _ze_event_state_entry *_event_state_get_or_add(ze_event_handle_t ev) {
  if (!ev)
    return NULL;
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (!e) {
    e = (struct _ze_event_state_entry *)calloc(1, sizeof(*e));
    if (!e)
      return NULL;
    e->ev = ev;
    HASH_ADD_PTR(_ze_event_state, ev, e);
  }
  return e;
}

/* Drop the entry if it carries nothing worth keeping (no latest slot, no
 * stashed kts) — keeps the map bounded as facts are cleared. */
static inline void _event_state_gc(struct _ze_event_state_entry *e) {
  if (e && !e->latest && !e->has_kts) {
    HASH_DEL(_ze_event_state, e);
    free(e);
  }
}

static inline struct _ze_slot *_event_latest_get(ze_event_handle_t ev) {
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  return e ? e->latest : NULL;
}

static inline void _event_latest_set(ze_event_handle_t ev, struct _ze_slot *slot) {
  struct _ze_event_state_entry *e = _event_state_get_or_add(ev);
  if (e)
    e->latest = slot;
}

/* Clear latest iff it still points at `slot` (a newer Append may own it now). */
static inline void _event_latest_clear_if(ze_event_handle_t ev, struct _ze_slot *slot) {
  if (!ev)
    return;
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (e && e->latest == slot) {
    e->latest = NULL;
    _event_state_gc(e);
  }
}

static inline void _event_kts_set(ze_event_handle_t ev, ze_kernel_timestamp_result_t val) {
  struct _ze_event_state_entry *e = _event_state_get_or_add(ev);
  if (e) {
    e->kts = val;
    e->has_kts = true;
  }
}

static inline int _event_kts_get(ze_event_handle_t ev, ze_kernel_timestamp_result_t *out) {
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (!e || !e->has_kts)
    return 0;
  *out = e->kts;
  return 1;
}

/* Evict the whole entry (both facts) — called when the event is destroyed. */
static inline void _event_state_del(ze_event_handle_t ev) {
  if (!ev)
    return;
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (e) {
    HASH_DEL(_ze_event_state, e);
    free(e);
  }
}

/* Pop one recycled event wrapper from the per-context freelist; NULL
 * if none cached (caller falls back to creating a fresh L0 event). */
static struct _ze_event_h *_get_ze_event(ze_context_handle_t context) {
  struct _ze_event_pool_entry *pool = NULL;
  HASH_FIND_PTR(_ze_event_pools, &context, pool);
  if (!pool || !pool->events)
    return NULL;
  struct _ze_event_h *e = pool->events;
  DL_DELETE(pool->events, e);
  return e;
}

/* Return an event wrapper to its per-context freelist. On total failure
 * (no bucket can be allocated), destroy the backing L0 objects and free
 * the wrapper — we'd rather leak nothing than poison the freelist. */
static void _put_ze_event(struct _ze_event_h *val) {
  _ZE_MUST(ZE_EVENT_HOST_RESET_PTR(val->event));
  struct _ze_event_pool_entry *pool = NULL;
  HASH_FIND_PTR(_ze_event_pools, &val->context, pool);
  if (!pool) {
    pool = (struct _ze_event_pool_entry *)calloc(1, sizeof(*pool));
    if (!pool) {
      THAPI_DBGLOG("Failed to allocate memory");
      if (val->event_pool) {
        if (val->event)
          ZE_EVENT_DESTROY_PTR(val->event);
        ZE_EVENT_POOL_DESTROY_PTR(val->event_pool);
      }
      free(val);
      return;
    }
    pool->context = val->context;
    HASH_ADD_PTR(_ze_event_pools, context, pool);
  }
  DL_PREPEND(pool->events, val);
}

struct _ze_event_h *_ze_event_wrappers = NULL;

/* Get a zeroed event wrapper struct: pop from the global recycle list if
 * any, else calloc a fresh one. The wrapper is context-agnostic — only
 * the backing L0 event + pool inside it bind to a specific ctx. */
static struct _ze_event_h *_get_ze_event_wrapper(void) {
  struct _ze_event_h *e = _ze_event_wrappers;
  if (e)
    DL_DELETE(_ze_event_wrappers, e);
  else
    e = (struct _ze_event_h *)calloc(1, sizeof(*e));
  return e;
}

/* Return a wrapper struct to the recycle list. Used in two situations:
 *   1) wrapper construction failed, no L0 objects ever attached;
 *   2) the wrapper's context is being destroyed — caller has already
 *      arranged for the L0 event/pool inside to be released (or left
 *      them to die with the context).
 * We zero before publishing so a future _get_ze_event_wrapper returns
 * something equivalent to a fresh calloc. */
static void _put_ze_event_wrapper(struct _ze_event_h *val) {
  memset(val, 0, sizeof(*val));
  DL_PREPEND(_ze_event_wrappers, val);
}

/* Caller-supplied ctx avoids a redundant zeCommandListGetContextHandle
 * (the prologue already fetched it). L0 event/pool create runs under
 * the state mutex; cold path, bounded cost. */
static struct _ze_event_h *_get_profiling_event(ze_context_handle_t context) {
  struct _ze_event_h *e_w = _get_ze_event(context);
  if (e_w)
    return e_w;
  e_w = _get_ze_event_wrapper();
  if (!e_w) {
    THAPI_DBGLOG("Could not create a new event wrapper for context: %p", context);
    return NULL;
  }

  ze_event_pool_desc_t desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL,
      ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(context, &desc, 0, NULL, &e_w->event_pool);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventPoolCreate failed with %d, for context: %p", res, context);
    goto cleanup_wrapper;
  }
  ze_event_desc_t e_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, NULL, 0, ZE_EVENT_SCOPE_FLAG_HOST,
                            ZE_EVENT_SCOPE_FLAG_HOST};
  res = ZE_EVENT_CREATE_PTR(e_w->event_pool, &e_desc, &e_w->event);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventCreate failed with %d, for event pool: %p, context: %p", res,
                 e_w->event_pool, context);
    goto cleanup_ep;
  }
  return e_w;
cleanup_ep:
  ZE_EVENT_POOL_DESTROY_PTR(e_w->event_pool);
cleanup_wrapper:
  _put_ze_event_wrapper(e_w);
  return NULL;
}

/* Unlink chunk c from cl_data->chunks and free the struct. Slot-side cleanup
 * (events, waits, preds) is the caller's responsibility — this helper only
 * owns the chunk envelope. */
static void
_cl_chunk_free(struct _ze_command_list_obj_data *cl_data, struct _ze_slot_chunk *c) {
  DL_DELETE(cl_data->chunks, c);
  free(c);
}

/* Allocate a new chunk and append it to cl_data->chunks. */
static struct _ze_slot_chunk *_cl_chunk_alloc(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_chunk *c = (struct _ze_slot_chunk *)calloc(1, sizeof(*c));
  if (!c)
    return NULL;
  DL_APPEND(cl_data->chunks, c);
  return c;
}

/* Allocate one new slot at the tail of cl_data->chunks. Grows by one
 * chunk for imm cls; regular cls stay at one chunk and return NULL when
 * full (their inj-signal swap and user-signal barrier are baked into the
 * closed cl body per Append, so a post-Close chunk can't be addressed). */
static struct _ze_slot *_cl_slot_append(struct _ze_command_list_obj_data *cl_data,
                                        struct _ze_event_h *inj,
                                        ze_event_handle_t attr,
                                        ze_event_handle_t *waits,
                                        uint32_t n_waits) {
  struct _ze_slot_chunk *tail = cl_data->chunks ? cl_data->chunks->prev : NULL;
  if (!tail || tail->n_used >= _ZE_CHUNK_SLOTS) {
    if (tail && !cl_data->is_immediate) {
      /* Regular cl is capped at one chunk (baked into the closed body). Past
       * the cap we drop the Append's profiling silently — warn once so the
       * data loss is at least visible. Called under _ze_state_mutex. */
      static int warned = 0;
      if (!warned) {
        warned = 1;
        _THAPI_LOG("warning: regular command list %p exceeded %d profiled "
                   "Appends in one build; further Appends will not be timed",
                   (void *)cl_data->ptr, _ZE_CHUNK_SLOTS);
      }
      return NULL;
    }
    tail = _cl_chunk_alloc(cl_data);
    if (!tail)
      return NULL;
  }
  uint32_t idx = tail->n_used;
  struct _ze_slot *s = &tail->slots[idx];
  /* Chunk memory is calloc'd, so all other slot fields are already zero. */
  s->owner = cl_data;
  s->chunk = tail;
  s->inj = inj;
  s->attr = attr;
  if (n_waits) {
    s->waits = (ze_event_handle_t *)malloc(n_waits * sizeof(ze_event_handle_t));
    if (s->waits) {
      memcpy(s->waits, waits, n_waits * sizeof(ze_event_handle_t));
      s->n_waits = n_waits;
    }
  }
  tail->n_used++;
  tail->n_held++;
  return s;
}

/* Compute s->preds from s->waits via the global event_latest_signaled
 * map, plus the previous live slot on this cl if the cl is in-order.
 * Marks s live and publishes s as the new event_latest_signaled[attr]. */
static void _slot_instantiate(struct _ze_command_list_obj_data *cl_data, struct _ze_slot *s) {
  /* Slot must be inert: live=0, preds NULL. Re-instantiating a live slot
   * would overwrite preds[] (leaking the prior pred refs) and let the
   * in-order pred walk pick up later-appended live siblings as predecessors,
   * forming cycles that infinite-loop _slot_drain. */
  _THAPI_ASSERT(!s->live, "slot %p already live (double _slot_instantiate)", (void *)s);
  s->live = true;
  cl_data->n_live++;
  uint32_t cap = s->n_waits + 1; /* +1 for in-order prev */
  s->preds = (struct _ze_slot **)calloc(cap, sizeof(struct _ze_slot *));
  s->n_preds = 0;
  for (uint32_t i = 0; i < s->n_waits; ++i) {
    struct _ze_slot *p = _event_latest_get(s->waits[i]);
    if (p && p->live)
      s->preds[s->n_preds++] = p;
  }
  if (cl_data->is_in_order) {
    /* Walk chunks newest-to-oldest, slots high-to-low, stop at the first
     * live slot strictly before s. Chunks are appended in time order
     * (DL_APPEND) and slots within a chunk in time order, so reverse-walk
     * yields reverse time order. Skip s itself; s might still have
     * live=0 here but the !=s guard is safe and clearer. */
    struct _ze_slot_chunk *c;
    struct _ze_slot *prev = NULL;
    for (c = cl_data->chunks ? cl_data->chunks->prev : NULL; c && !prev;
         c = (c == cl_data->chunks) ? NULL : c->prev) {
      for (int32_t i = (int32_t)c->n_used - 1; i >= 0; --i) {
        if (&c->slots[i] == s)
          continue;
        if (c->slots[i].live) {
          prev = &c->slots[i];
          break;
        }
      }
    }
    if (prev)
      s->preds[s->n_preds++] = prev;
  }
  /* Each new pred edge holds a ref on its target. */
  for (uint32_t i = 0; i < s->n_preds; ++i)
    s->preds[i]->refs++;
  if (s->attr)
    _event_latest_set(s->attr, s);
}

/* Re-expose the user's signal event on the user cl: the prologue swapped
 * user_signal for inj (so the kernel now signals inj), and without re-signaling
 * user_signal the user's Sync(user_signal) would hang forever. No-op (returns
 * 0) when the user passed no signal; returns 1 when an op was appended.
 *
 * In-order cl: the cl executes strictly sequentially, so a plain
 * AppendSignalEvent(user_signal) placed right after the kernel already runs
 * after it — cheaper than a full pipeline barrier. Out-of-order cl: a bare
 * SignalEvent has no ordering vs the kernel, so we need AppendBarrier with
 * wait=inj to force user_signal to fire only after the kernel completed.
 *
 * Mutex-agnostic — it issues an L0 Append on the user cl and touches no tracer
 * state, so it is correct both inside the critical section (normal path) and
 * outside it (the failure-path compensation). Aborts on L0 failure (a silent
 * hang is worse). */
static int _chain_user_signal(ze_command_list_handle_t command_list,
                              ze_event_handle_t inj_event,
                              ze_event_handle_t user_signal,
                              int in_order) {
  if (!user_signal)
    return 0;
  if (in_order)
    _ZE_MUST(ZE_COMMAND_LIST_APPEND_SIGNAL_EVENT_PTR(command_list, user_signal));
  else
    _ZE_MUST(ZE_COMMAND_LIST_APPEND_BARRIER_PTR(command_list, user_signal, 1, &inj_event));
  return 1;
}

/* Roll back the slot just handed out by _cl_slot_append. We were the last to
 * touch the tail chunk and hold _ze_state_mutex, so decrementing n_used/n_held
 * and zeroing the slot is safe; if the chunk was freshly allocated only for
 * this Append (n_used now 0), free it back so a slot-append failure doesn't
 * leak a chunk. */
static void _slot_append_rollback(struct _ze_command_list_obj_data *cl_data, struct _ze_slot *s) {
  free(s->waits);
  struct _ze_slot_chunk *c = s->chunk;
  c->n_used--;
  c->n_held--;
  memset(s, 0, sizeof(*s));
  if (c->n_used == 0)
    _cl_chunk_free(cl_data, c);
}

/* Append-time hook from profiling_epilogue. The prologue swapped user's
 * hSignalEvent for inj->event; user_signal is the original (possibly NULL),
 * user_waits is the user's wait list, ctx is the cl's context (fetched
 * once in the prologue, threaded in).
 *
 * One uniform path for all cls: the kernel/copy signals inj (a
 * KERNEL_TIMESTAMP event we own), so inj carries the real kernel timing; drain
 * host-reads it via zeEventQueryKernelTimestamp. _chain_user_signal re-exposes
 * the user's original signal (skipped when NULL): a SignalEvent for in-order
 * cls, a Barrier(wait=inj) for out-of-order. Host-read works for any signaling
 * engine (compute or copy). */
static void _universal_record_append(ze_command_list_handle_t command_list,
                                     ze_context_handle_t ctx,
                                     struct _ze_event_h *inj,
                                     ze_event_handle_t user_signal,
                                     ze_event_handle_t *user_waits,
                                     uint32_t user_n_waits) {
  if (!inj || !ctx)
    return;
  struct _ze_slot *s = NULL;
  int barrier_chained = 0;
  int in_order = 0;

  inj->context = ctx;

  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find(command_list);
  if (!cl_data)
    goto fail_locked;
  in_order = cl_data->is_in_order;

  /* Publish the cl->ctx mapping. _on_destroy_context's per-cl sweep matches
   * against it. */
  cl_data->cached_context = ctx;

  s = _cl_slot_append(cl_data, inj, user_signal, user_waits, user_n_waits);
  if (!s)
    goto fail_locked;

  /* Re-expose the user's signal (the prologue swapped user_signal for inj).
   * In-order: a plain SignalEvent after the kernel; OOO: a barrier waiting on
   * inj. No-op when user_signal is NULL. */
  barrier_chained = _chain_user_signal(command_list, inj->event, user_signal, in_order);
  _slot_instantiate(cl_data, s);
  pthread_mutex_unlock(&_ze_state_mutex);
  return;

fail_locked:
  if (s)
    _slot_append_rollback(cl_data, s);
  _put_ze_event(inj);
  pthread_mutex_unlock(&_ze_state_mutex);
  /* Compensate outside the state mutex: if we bailed before chaining
   * user_signal off inj, do it now or the user's Sync(user_signal) hangs. */
  if (!barrier_chained)
    _chain_user_signal(command_list, inj->event, user_signal, in_order);
}

/* Dispose the per-slot resources shared by every teardown path: the inj
 * event, the waits[] copy, the preds[] array, and the slot's entry in
 * event_latest_signaled. The event-disposal target differs by caller:
 *   _ZE_DISPOSE_POOL    -> _put_ze_event (ctx alive: events recycle to the pool)
 *   _ZE_DISPOSE_WRAPPER -> _put_ze_event_wrapper (ctx dying: only recycle the
 *                          wrapper struct; the L0 event/pool die with the ctx)
 * Deliberately does NOT touch chunk accounting (n_held / n_pinned), refs,
 * owner, or live — those are caller-specific and stay at the call site.
 * Every field is nulled so the call is idempotent (safe to re-run on a slot
 * whose preds/latest-signaled were already cleared during drain). */
enum _ze_slot_dispose_mode { _ZE_DISPOSE_POOL, _ZE_DISPOSE_WRAPPER };
static void _slot_dispose_resources(struct _ze_slot *s, enum _ze_slot_dispose_mode mode) {
  if (s->inj) {
    if (mode == _ZE_DISPOSE_WRAPPER)
      _put_ze_event_wrapper(s->inj);
    else
      _put_ze_event(s->inj);
    s->inj = NULL;
  }
  free(s->waits);
  s->waits = NULL;
  s->n_waits = 0;
  free(s->preds);
  s->preds = NULL;
  s->n_preds = 0;
  _event_latest_clear_if(s->attr, s);
  s->attr = NULL;
}

/* Reclaim a slot: PUT events back to the per-context pool, free waits,
 * decrement chunk n_held; if the chunk hits 0 AND isn't the active
 * tail, unlink and free it. Regular cls are skipped (their inj is
 * baked into the cl body — reclaim happens at cl destroy instead). */
static void _slot_release(struct _ze_slot *s) {
  if (!s)
    return;
  /* Detached slot: its owning cl was torn down (reset/destroy) while this
   * slot was still a pred of a live slot elsewhere. Its resources were freed
   * at reclaim and owner was nulled; the chunk struct was kept alive only to
   * keep this slot's refs addressable. We are the downstream drain dropping
   * the last ref — drop the chunk's pin and free the bare struct at zero. */
  if (!s->owner && s->chunk && s->chunk->n_pinned) {
    struct _ze_slot_chunk *c = s->chunk;
    if (--c->n_pinned == 0)
      free(c);
    return;
  }
  if (!s->owner || !s->owner->is_immediate)
    return;
  /* Reached only from _slot_drain, which already freed s->preds and cleared
   * event_latest_signaled[s->attr]; the primitive re-running those is a no-op
   * (free(NULL); _clear_if on a missing/overwritten key does nothing). */
  _slot_dispose_resources(s, _ZE_DISPOSE_POOL);

  struct _ze_slot_chunk *c = s->chunk;
  struct _ze_command_list_obj_data *cl = s->owner;
  if (!c)
    return;
  c->n_held--;
  if (c->n_held == 0 && c != cl->chunks->prev)
    _cl_chunk_free(cl, c);
}

/* Drain one slot. Recurses on its preds, emits the slot's tracepoint,
 * drops one ref on each pred (releasing fully-drained-and-unreferenced
 * preds), then releases s if its own refs hit 0. Safe to call on an
 * already-drained (live=0) slot.
 *
 * No cycle guard: preds come from in-order prev (strictly earlier slot
 * in the same cl, DAG) and from event_latest_signaled[wait_event] (a
 * slot published BEFORE us). A cycle would need user-declared mutual
 * waits, which L0 itself deadlocks on. */
static void _slot_drain(struct _ze_slot *s) {
  if (!s || !s->live)
    return;
  for (uint32_t i = 0; i < s->n_preds; ++i)
    _slot_drain(s->preds[i]);
  s->live = false;
  if (s->owner)
    s->owner->n_live--;
  ze_event_handle_t attr = s->attr ? s->attr : (s->inj ? s->inj->event : NULL);
  /* Host-read the kernel timing from OUR injected KERNEL_TIMESTAMP event. Drain
   * is always reached after a queue/cl/event sync that already completed the
   * work (for a resubmitted regular cl, the next Execute's force-sync drains the
   * prior instance before it re-signals inj), so the query returns immediately;
   * the HostSynchronize is a cheap guard against under-synced user code. inj is
   * OURS — we never sync/query the user's event, only stash under it below. */
  int have_result = 0;
  ze_kernel_timestamp_result_t r;
  if (s->inj && s->inj->event) {
    _ZE_MUST(ZE_EVENT_HOST_SYNCHRONIZE_PTR(s->inj->event, UINT64_MAX));
    _ZE_MUST(ZE_EVENT_QUERY_KERNEL_TIMESTAMP_PTR(s->inj->event, &r));
    have_result = 1;
  }
  if (have_result && attr) {
    /* Stash the kernel result under the user's own event so the user's
     * zeEventQueryKernelTimestamp returns kernel timing, not the barrier op
     * timing their event actually carries (we swapped it for inj). Only when
     * the user supplied an event (s->attr); inj is ours, not queryable. */
    if (s->attr)
      _event_kts_set(s->attr, r);
    if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results))
      do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, attr, ZE_RESULT_SUCCESS,
                    ZE_RESULT_SUCCESS, r.global.kernelStart, r.global.kernelEnd,
                    r.context.kernelStart, r.context.kernelEnd);
  }
  /* Reset our inj now that we've read it, so it starts clean for its next use.
   * We never touch the user's event — only our own inj. Safe against a later
   * drain of this same slot: a slot is only re-drained after being
   * re-instantiated at a subsequent Execute, which re-signals inj first (see
   * _on_execute_one_cl); the final drain of a slot is never followed by another.
   * (Immediate cls dispose inj to the pool at _slot_release, which also resets
   * it — so only regular, kept-across-replay slots need the reset here.) */
  if (s->inj && s->inj->event && s->owner && !s->owner->is_immediate)
    _ZE_MUST(ZE_EVENT_HOST_RESET_PTR(s->inj->event));
  _event_latest_clear_if(s->attr, s);
  /* Drop refs on preds; release any that hit 0 and are already drained. */
  for (uint32_t i = 0; i < s->n_preds; ++i) {
    struct _ze_slot *p = s->preds[i];
    if (--p->refs == 0 && !p->live)
      _slot_release(p);
  }
  free(s->preds);
  s->preds = NULL;
  s->n_preds = 0;
  if (s->refs == 0)
    _slot_release(s);
}

/* Drain every live slot in a cl (walk chunks oldest-to-newest, slots
 * low-to-high — natural time order for emission). */
static void _cl_drain(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_chunk *c, *tmp;
  DL_FOREACH_SAFE (cl_data->chunks, c, tmp) {
    /* Bump refcount during traversal so the last _slot_drain doesn't
     * free c out from under the inner loop. Drop after, free here. */
    c->n_held++;
    for (uint32_t i = 0; i < c->n_used; ++i)
      _slot_drain(&c->slots[i]);
    c->n_held--;
    if (c->n_held == 0 && c != cl_data->chunks->prev)
      _cl_chunk_free(cl_data, c);
  }
  _cl_index_clear(cl_data);
  cl_data->in_flight_q = NULL;
  cl_data->in_flight_fence = NULL;
}

/* Reclaim one chunk during cl teardown (reset or single-cl destroy, ctx
 * alive). Releases every slot's resources (events to pool, waits, preds,
 * clears latest-signaled), then either frees the chunk or — if any slot is
 * still referenced as a pred by a live slot in ANOTHER cl (refs>0) — DETACHES
 * it: unlink from cl_data->chunks, null each slot's owner, and keep the bare
 * struct alive with n_pinned = #referenced slots. The downstream drains that
 * drop those refs free the struct (see _slot_release's detached branch).
 * Without this, freeing the chunk here would dangle the referrers' preds[]. */
static void _cl_chunk_reclaim(struct _ze_command_list_obj_data *cl_data, struct _ze_slot_chunk *c) {
  uint32_t pinned = 0;
  for (uint32_t i = 0; i < c->n_used; ++i) {
    struct _ze_slot *s = &c->slots[i];
    _slot_dispose_resources(s, _ZE_DISPOSE_POOL);
    if (s->refs)
      pinned++;
  }
  if (pinned == 0) {
    _cl_chunk_free(cl_data, c);
    return;
  }
  /* Detach: keep the struct alive for the surviving referenced slots. */
  DL_DELETE(cl_data->chunks, c);
  for (uint32_t i = 0; i < c->n_used; ++i)
    c->slots[i].owner = NULL;
  c->n_pinned = pinned;
}

/* Reclaim (or detach) every chunk of a cl whose ctx is still alive, plus clear
 * its in-flight index membership. Shared by reset (keep cl_data) and single-cl
 * destroy (free cl_data after). Detaches any chunk with slots still referenced
 * cross-cl; see _cl_chunk_reclaim. */
static void _cl_reclaim_chunks(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_chunk *c, *tmp;
  DL_FOREACH_SAFE (cl_data->chunks, c, tmp)
    _cl_chunk_reclaim(cl_data, c);
  _cl_index_clear(cl_data);
}

/* Reclaim all of a regular cl's slot state, keeping cl_data registered and
 * empty for reuse. Used by the zeCommandListReset hook. */
static void _cl_data_reset(struct _ze_command_list_obj_data *cl_data) {
  _cl_reclaim_chunks(cl_data);
  cl_data->n_live = 0;
  cl_data->in_flight_q = NULL;
  cl_data->in_flight_fence = NULL;
}

/* Release everything cl_data owns and free cl_data itself. Caller has
 * already removed cl_data from _ze_cls (single-cl: _cl_find_and_del;
 * per-ctx sweep: HASH_DEL inside the iter). ctx alive: reclaim per-chunk
 * (detaching chunks whose slots are still referenced cross-cl), exactly like
 * reset. ctx dying: no slot can outlive the ctx (no detach needed) and the L0
 * event/pool die with the ctx, so we only recycle the wrapper structs
 * (_ZE_DISPOSE_WRAPPER) and drop the index in bulk elsewhere. */
static void _cl_data_destroy(struct _ze_command_list_obj_data *cl_data, int ctx_dying) {
  if (!ctx_dying) {
    _cl_reclaim_chunks(cl_data);
    free(cl_data);
    return;
  }
  _cl_index_clear(cl_data);
  struct _ze_slot_chunk *c, *tmp;
  DL_FOREACH_SAFE (cl_data->chunks, c, tmp) {
    for (uint32_t i = 0; i < c->n_used; ++i)
      _slot_dispose_resources(&c->slots[i], _ZE_DISPOSE_WRAPPER);
    _cl_chunk_free(cl_data, c);
  }
  free(cl_data);
}

/* zeCommandListDestroy epilogue. Per L0 spec the device is no longer
 * referencing the cl, so we don't drain — just release our state.
 * Regular cls recycle inj here (cl body is about to die anyway);
 * immediate cls' slots are typically already released at drain. */
static void _on_destroy_command_list(ze_command_list_handle_t command_list) {
  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find_and_del(command_list);
  if (cl_data)
    _cl_data_destroy(cl_data, /*ctx_dying=*/0);
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* zeCommandListReset epilogue. The L0 spec requires the user to have
 * synchronized before Reset, so our slots are drained — but for a REGULAR cl
 * "drained" is not "reclaimed": _slot_release is a no-op for regular cls
 * (their inj is baked into the cl body, kept for reuse across Executes), so
 * the slots linger. Reset wipes that body, so we must reclaim now; otherwise
 * the stale slots are re-published on the next Execute (massive over-count)
 * and their chunks accumulate (leak). We drain defensively first in case the
 * user under-synced, then reclaim. The cl stays registered, empty for reuse. */
static void _on_reset_command_list(ze_command_list_handle_t command_list) {
  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find(command_list);
  if (cl_data) {
    _cl_drain(cl_data);
    _cl_data_reset(cl_data);
  }
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* zeContextDestroy prologue. Two sweeps to drop our own L0 objects
 * that live inside this ctx; the user's own cls/events are their
 * responsibility per the L0 contract. */
static void _on_destroy_context(ze_context_handle_t hContext) {
  /* 1) Drop cls bound to this ctx. */
  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = NULL, *cl_tmp = NULL;
  HASH_ITER (hh, _ze_cls, cl_data, cl_tmp) {
    if (cl_data->cached_context != hContext)
      continue;
    HASH_DEL(_ze_cls, cl_data);
    _cl_data_destroy(cl_data, /*ctx_dying=*/1);
  }

  /* 2) Per-ctx event pool freelist. */
  struct _ze_event_pool_entry *pe = NULL;
  HASH_FIND_PTR(_ze_event_pools, &hContext, pe);
  if (pe) {
    HASH_DEL(_ze_event_pools, pe);
    struct _ze_event_h *w, *w_tmp;
    DL_FOREACH_SAFE (pe->events, w, w_tmp) {
      if (w->event)
        ZE_EVENT_DESTROY_PTR(w->event);
      if (w->event_pool)
        ZE_EVENT_POOL_DESTROY_PTR(w->event_pool);
      DL_DELETE(pe->events, w);
      _put_ze_event_wrapper(w);
    }
    free(pe);
  }
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* The four user sync APIs all reduce to "drain the slots the synced anchor
 * covers". They differ only in how the anchor selects work:
 *
 *   _ZE_SYNC_CL     zeCommandListHostSynchronize  -> the one named cl
 *   _ZE_SYNC_QUEUE  zeCommandQueueSynchronize     -> every cl with in_flight_q == h
 *   _ZE_SYNC_FENCE  zeFenceHostSynchronize        -> every cl with in_flight_fence == h
 *   _ZE_SYNC_EVENT  zeEventHostSynchronize        -> the slot that last signaled h,
 *                                                    walking its pred edges
 *
 * QUEUE/FENCE share one rule: a queue/fence wait completes exactly the cls a
 * given Execute submitted, identified by the handle stamped on the cl at
 * Execute. CL/EVENT name their target directly. Draining reclaims each slot's
 * bookkeeping via _slot_release. For the cl/queue/fence anchors _cl_drain
 * already cleared in_flight_*, while the event anchor may leave live siblings,
 * so it clears in_flight_* only once the cl has no slot left in flight. */
enum _ze_sync_kind { _ZE_SYNC_CL, _ZE_SYNC_QUEUE, _ZE_SYNC_FENCE, _ZE_SYNC_EVENT };
static void _on_sync(enum _ze_sync_kind kind, void *h) {
  pthread_mutex_lock(&_ze_state_mutex);
  if (kind == _ZE_SYNC_EVENT) {
    struct _ze_slot *s = _event_latest_get((ze_event_handle_t)h);
    if (s && s->owner) {
      _slot_drain(s);
      if (!s->owner->n_live) {
        _cl_index_clear(s->owner);
        s->owner->in_flight_q = NULL;
        s->owner->in_flight_fence = NULL;
      }
    }
  } else if (kind == _ZE_SYNC_CL) {
    struct _ze_command_list_obj_data *cl_data = _cl_find((ze_command_list_handle_t)h);
    if (cl_data)
      _cl_drain(cl_data);
  } else { /* _ZE_SYNC_QUEUE / _ZE_SYNC_FENCE: drain just the indexed cls */
    struct _ze_inflight_bucket *b = NULL;
    if (kind == _ZE_SYNC_QUEUE)
      HASH_FIND_PTR(_ze_q_index, &h, b);
    else
      HASH_FIND_PTR(_ze_fence_index, &h, b);
    if (b) {
      struct _ze_command_list_obj_data *cl_data = NULL, *tmp = NULL;
      /* SAFE2 because _cl_drain -> _cl_index_clear unlinks cl_data from this
       * very bucket (and may free the bucket on the last unlink). */
      if (kind == _ZE_SYNC_QUEUE) {
        DL_FOREACH_SAFE2 (b->cls, cl_data, tmp, q_next)
          _cl_drain(cl_data);
      } else {
        DL_FOREACH_SAFE2 (b->cls, cl_data, tmp, f_next)
          _cl_drain(cl_data);
      }
    }
  }
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* zeEventQueryKernelTimestamp epilogue. If we drained a kernel result for
 * this user event, overwrite *dstptr with it: the user's event carries the
 * barrier/signal op timing (we swapped their signal for inj at Append), but the
 * caller wants the KERNEL timing, which we stashed at drain. Returns 1 if it
 * served a stashed result. */
static int _on_query_kernel_timestamp(ze_event_handle_t hEvent,
                                      ze_kernel_timestamp_result_t *dstptr) {
  if (!hEvent || !dstptr)
    return 0;
  pthread_mutex_lock(&_ze_state_mutex);
  int found = _event_kts_get(hEvent, dstptr);
  pthread_mutex_unlock(&_ze_state_mutex);
  return found;
}

/* zeEventDestroy epilogue (success only). The per-event state entry is keyed by
 * the event's HANDLE ADDRESS, which the L0 driver recycles: a fresh event
 * created after this one is destroyed can land on the same address. Without
 * eviction the new event inherits the dead one's entry —
 *   .kts:    a never-signaled event's zeEventQueryKernelTimestamp would be
 *            served the prior event's stale timing;
 *   .latest: a wait on the reused address would resolve to a freed slot, a
 *            use-after-free in the pred walk.
 * Evicting the entry at destroy bounds the map to live events and closes the
 * recycled-address reads. Gated on a successful destroy by the caller: a failed
 * destroy leaves the event (and its address) alive, so its data stays. */
static void _on_destroy_event(ze_event_handle_t hEvent) {
  pthread_mutex_lock(&_ze_state_mutex);
  _event_state_del(hEvent);
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* Execute-PROLOGUE handler for ONE cl. Runs BEFORE L0 submits this Execute, as
 * one critical section (so concurrent Executes/Syncs observe in_flight_q
 * atomically). Three steps: */
static void _on_execute_one_cl(ze_command_queue_handle_t hQueue,
                               ze_fence_handle_t hFence,
                               ze_command_list_handle_t command_list) {
  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find(command_list);
  if (!cl_data) {
    pthread_mutex_unlock(&_ze_state_mutex);
    return;
  }

  /* 1) If the cl is still in flight from a prior Execute, force-sync that queue
   *    and drain it — reading each slot's timing from inj and resetting inj —
   *    BEFORE this submission re-signals the same baked inj. Running this in the
   *    PROLOGUE (not after submit) is what keeps a replayed regular cl's #N-1
   *    timing from being clobbered by #N (reg_Event_09), and serializes the
   *    same cl reused from another thread (multithreaded_01). Syncing a QUEUE
   *    (never a user event) only observes already-submitted work. */
  if (cl_data->in_flight_q) {
    _ZE_MUST(ZE_COMMAND_QUEUE_SYNCHRONIZE_PTR(cl_data->in_flight_q, UINT64_MAX));
    _cl_drain(cl_data);
  }
  /* 2) Re-instantiate each not-yet-live slot into the dep graph. A regular cl
   *    is replayed: its inj-signal swap and user-signal chain are baked in the
   *    closed body and re-fire automatically, so a drained slot just needs its
   *    dep-graph entry rebuilt; a live slot (first Execute after Append) is left
   *    as is. */
  struct _ze_slot_chunk *c;
  DL_FOREACH (cl_data->chunks, c) {
    for (uint32_t j = 0; j < c->n_used; ++j) {
      struct _ze_slot *slot = &c->slots[j];
      if (!slot->inj || slot->live)
        continue;
      _slot_instantiate(cl_data, slot);
    }
  }
  /* 3) Stamp in_flight_q / in_flight_fence and index the cl under its queue
   *    (and fence) so a later queue/fence sync drains it without scanning every
   *    live cl. Step 1 already unlinked any prior in-flight membership. */
  cl_data->in_flight_q = hQueue;
  cl_data->in_flight_fence = hFence;
  _cl_index_set(cl_data, hQueue, hFence);

  pthread_mutex_unlock(&_ze_state_mutex);
}

static void _on_execute_command_lists_prologue(uint32_t numCommandLists,
                                               ze_command_list_handle_t *phCommandLists,
                                               ze_command_queue_handle_t hQueue,
                                               ze_fence_handle_t hFence) {
  for (uint32_t i = 0; i < numCommandLists; ++i)
    _on_execute_one_cl(hQueue, hFence, phCommandLists[i]);
}

/* ========================================================================
 * Property/info dumping + tracer init
 *
 * Separate concern from the slot/drain engine above: read device/driver/
 * kernel/memory properties and emit the lttng_ust_ze_properties / _build
 * tracepoints, plus one-time loader/symbol init. Self-contained — the
 * engine never calls into this section, and the only external callers are
 * ze_model.rb hooks (_do_state, _dump_memory_info,
 * _dump_command_list_device_timer, _in_loader_init) and gen_ze.rb
 * (_init_tracer / _init_tracer_dump).
 * ======================================================================== */

static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int _in_init = 0;
static volatile unsigned int _in_loader_init = 0;
static volatile unsigned int _initialized = 0;

static pthread_once_t _init_dump = PTHREAD_ONCE_INIT;
static __thread volatile int _in_init_dump = 0;
static volatile unsigned int _initialized_dump = 0;

static inline int _do_state() {
  return _do_profile || tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
         tracepoint_enabled(lttng_ust_ze_properties, memory_info_range);
}

static void _dump_driver_subdevice_properties(ze_driver_handle_t hDriver,
                                              ze_device_handle_t hDevice) {
  if (!tracepoint_enabled(lttng_ust_ze_properties, subdevice))
    return;

  uint32_t subDeviceCount = 0;
  if (ZE_DEVICE_GET_SUB_DEVICES_PTR(hDevice, &subDeviceCount, NULL) != ZE_RESULT_SUCCESS ||
      subDeviceCount == 0)
    return;
  ze_device_handle_t *phSubDevices =
      (ze_device_handle_t *)alloca(subDeviceCount * sizeof(ze_device_handle_t));

  if (ZE_DEVICE_GET_SUB_DEVICES_PTR(hDevice, &subDeviceCount, phSubDevices) != ZE_RESULT_SUCCESS)
    return;

  for (uint32_t j = 0; j < subDeviceCount; j++) {
    ze_device_properties_t props = {0};
    props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    props.pNext = NULL;
    if (ZE_DEVICE_GET_PROPERTIES_PTR(phSubDevices[j], &props) == ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, subdevice, hDriver, hDevice, phSubDevices[j], &props);
  }
  return;
}

static void _dump_device_timer(ze_device_handle_t hDevice) {
  uint64_t hostTimestamp, deviceTimestamp;
  if (ZE_DEVICE_GET_GLOBAL_TIMESTAMPS_PTR(hDevice, &hostTimestamp, &deviceTimestamp) ==
      ZE_RESULT_SUCCESS)
    do_tracepoint(lttng_ust_ze_properties, device_timer, hDevice, hostTimestamp, deviceTimestamp);
}

static void _dump_command_list_device_timer(ze_command_list_handle_t hCommandList) {
  ze_device_handle_t hDevice = NULL;
  if (ZE_COMMAND_LIST_GET_DEVICE_HANDLE_PTR(hCommandList, &hDevice) == ZE_RESULT_SUCCESS && hDevice)
    _dump_device_timer(hDevice);
}

static void _dump_driver_device_properties(ze_driver_handle_t hDriver) {
  uint32_t deviceCount = 0;
  if (ZE_DEVICE_GET_PTR(hDriver, &deviceCount, NULL) != ZE_RESULT_SUCCESS || deviceCount == 0)
    return;
  ze_device_handle_t *phDevices =
      (ze_device_handle_t *)alloca(deviceCount * sizeof(ze_device_handle_t));

  if (ZE_DEVICE_GET_PTR(hDriver, &deviceCount, phDevices) != ZE_RESULT_SUCCESS)
    return;

  for (uint32_t i = 0; i < deviceCount; i++) {
    if (tracepoint_enabled(lttng_ust_ze_properties, device)) {
      ze_device_properties_t props = {0};
      props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
      props.pNext = NULL;
      if (ZE_DEVICE_GET_PROPERTIES_PTR(phDevices[i], &props) == ZE_RESULT_SUCCESS)
        do_tracepoint(lttng_ust_ze_properties, device, hDriver, phDevices[i], &props);
    }
    if (ZE_DEVICE_GET_GLOBAL_TIMESTAMPS_PTR &&
        tracepoint_enabled(lttng_ust_ze_properties, device_timer))
      _dump_device_timer(phDevices[i]);
    _dump_driver_subdevice_properties(hDriver, phDevices[i]);
  }
}

static void _dump_kernel_properties(ze_kernel_handle_t hKernel) {
  ze_kernel_properties_t kernelProperties;
  kernelProperties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  kernelProperties.pNext = NULL;
  if (ZE_KERNEL_GET_PROPERTIES_PTR(hKernel, &kernelProperties) == ZE_RESULT_SUCCESS)
    tracepoint(lttng_ust_ze_properties, kernel, hKernel, &kernelProperties);
}

static void _dump_properties() {
  uint32_t driverCount = 0;
  if (ZE_DRIVER_GET_PTR(&driverCount, NULL) != ZE_RESULT_SUCCESS || driverCount == 0)
    return;
  ze_driver_handle_t *phDrivers =
      (ze_driver_handle_t *)alloca(driverCount * sizeof(ze_driver_handle_t));
  if (ZE_DRIVER_GET_PTR(&driverCount, phDrivers) != ZE_RESULT_SUCCESS)
    return;
  if (tracepoint_enabled(lttng_ust_ze_properties, driver)) {
    for (uint32_t i = 0; i < driverCount; i++) {
      ze_driver_properties_t props = {0};
      props.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
      props.pNext = NULL;
      if (ZE_DRIVER_GET_PROPERTIES_PTR(phDrivers[i], &props) == ZE_RESULT_SUCCESS)
        do_tracepoint(lttng_ust_ze_properties, driver, phDrivers[i], &props);
    }
  }
  for (uint32_t i = 0; i < driverCount; i++)
    _dump_driver_device_properties(phDrivers[i]);
}

static void _dump_build_log(ze_module_build_log_handle_t hBuildLog) {
  size_t size;
  char *buildLog;
  ze_result_t res;

  res = ZE_MODULE_BUILD_LOG_GET_STRING_PTR(hBuildLog, &size, NULL);
  if (res != ZE_RESULT_SUCCESS)
    return;
  buildLog = (char *)malloc(size);
  if (!buildLog)
    return;
  res = ZE_MODULE_BUILD_LOG_GET_STRING_PTR(hBuildLog, &size, buildLog);
  if (res == ZE_RESULT_SUCCESS)
    do_tracepoint(lttng_ust_ze_build, log, buildLog);
  free(buildLog);
}

static inline void _dump_memory_info_ctx(ze_context_handle_t hContext, const void *ptr) {
  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties)) {
    ze_memory_allocation_properties_t memAllocProperties;
    memAllocProperties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
    memAllocProperties.pNext = NULL;
    ze_device_handle_t hDevice = NULL;
    if (ZE_MEM_GET_ALLOC_PROPERTIES_PTR(hContext, ptr, &memAllocProperties, &hDevice) ==
        ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, memory_info_properties, hContext, ptr,
                    &memAllocProperties, hDevice);
  }
  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
    void *base = NULL;
    size_t size = 0;
    if (ZE_MEM_GET_ADDRESS_RANGE_PTR(hContext, ptr, &base, &size) == ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, memory_info_range, hContext, ptr, base, size);
  }
}

static inline void _dump_memory_info(ze_command_list_handle_t hCommandList, const void *ptr) {
  ze_context_handle_t hContext = NULL;
  if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(hCommandList, &hContext) == ZE_RESULT_SUCCESS &&
      hContext)
    _dump_memory_info_ctx(hContext, ptr);
}

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;
  s = getenv("LTTNG_UST_ZE_LIBZE_LOADER");
  if (s)
    handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  else
    handle = dlopen("libze_loader.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  if (handle) {
    void *ptr = dlsym(handle, "zeInit");
    if (ptr == (void *)&zeInit) { // opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if (!handle) {
    fprintf(stderr, "Failure: could not load ze library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_ZE_VERBOSE");
  if (s)
    verbose = 1;

  s = getenv("LTTNG_UST_ZE_CHAINED_STRUCTS");
  if (s)
    _do_chained_structs = 1;

  s = getenv("LTTNG_UST_ZE_DDI_TABLE_FORWARD");
  if (s)
    _do_ddi_table_forward = 1;

  find_ze_symbols(handle, verbose);

  s = getenv("LTTNG_UST_ZE_PROFILE");
  if (s)
    _do_profile = 1;

  s = getenv("LTTNG_UST_ZE_PARANOID_DRIFT");
  if (s) {
    if (_do_profile)
      _do_paranoid_drift = 1;
    else if (verbose)
      fprintf(stderr,
              "Warning: LTTNG_UST_ZE_PARANOID_DRIFT not activated without LTTNG_UST_ZE_PROFILE\n");
  }

  s = getenv("LTTNG_UST_ZE_PARANOID_MEMORY_LOCATION");
  if (s)
    _do_paranoid_memory_location = 1;
}

static void _load_tracer_dump(void) {
  // FIX for intel tracing layer that needs to register its callbacks first...
  ZE_INIT_PTR(0);
  if (tracepoint_enabled(lttng_ust_ze_properties, driver) ||
      tracepoint_enabled(lttng_ust_ze_properties, device) ||
      tracepoint_enabled(lttng_ust_ze_properties, subdevice))
    _dump_properties();
}

static inline void _init_tracer(void) {
  if (__builtin_expect(_initialized, 1))
    return;
  /* Avoid reentrancy */
  if (!_in_init) {
    _in_init = 1;
    __sync_synchronize();
    pthread_once(&_init, _load_tracer);
    __sync_synchronize();
    _in_init = 0;
  }
  _initialized = 1;
}

static inline void _init_tracer_dump(void) {
  if (__builtin_expect(_initialized_dump, 1))
    return;
  /* Avoid reentrancy */
  if (!_in_init_dump) {
    _in_init_dump = 1;
    __sync_synchronize();
    pthread_once(&_init_dump, _load_tracer_dump);
    __sync_synchronize();
    _in_init_dump = 0;
  }
  _initialized_dump = 1;
}
