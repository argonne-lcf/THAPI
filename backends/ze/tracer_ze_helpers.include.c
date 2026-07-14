/* Timestamp capture
 * =================
 *
 * The goal is to measure the command submitted
 * The only way is to get a profiling event signaled by this command
 * The subtlety is that users are free to re-signal event
 *
 * We have 2 main scenarios:
 * 	1. If the user doesn't signal, we signal an event.
 * 	2. If the user signals, we intercept our event, and then signal their event
 *
 * At synchronization time, we look at the dependency chain to know which event of ours we can
 * query.
 *
 * Why it needs all this bookkeeping ?
 * --------------------------------------
 *
 *   1. The user syncs on more than events. Besides zeEventHostSynchronize they
 *      can sync a whole COMMAND LIST (or a queue, or a fence).
 *
 *   2. A command signaled on one cl can be waited on from another (phWaitEvents,
 *      cross-cl).
 *
 * So each profiled Append becomes a "slot" (its injected event + its dependency
 * edges). Slots are grouped per cl (for cl/queue/fence sync) but
 * linked by raw pointers across cls (for event/wait dependencies). Everything
 * below is the lifetime management that keeps those cross-cl pointers valid
 * exactly as long as someone can still reach them. See README.md
 * ("Timestamp profiling architecture") for the ownership / state / teardown
 * diagrams.
 *
 * Concurrency
 * -----------
 * One global mutex (_ze_state_mutex) covers all tracer state. Per-cl mutexes
 * don't work because draining a synced event follows dependency edges that
 * cross command lists, so a drain started from one cl mutates another. One
 * global mutex sidesteps the cross-cl ordering problem entirely.
 */

//         _   __
//     |  / \ /__
//     |_ \_/ \_|
//
#define THAPI_LOG(fmt, ...)                                                                        \
  do {                                                                                             \
    fprintf(stderr, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);                 \
    fflush(stderr);                                                                                \
  } while (0)

#ifdef THAPI_DEBUG
#define THAPI_DBGLOG(fmt, ...) THAPI_LOG(fmt, ##__VA_ARGS__)
#else
#define THAPI_DBGLOG(...)                                                                          \
  do {                                                                                             \
  } while (0)
#endif

/* Tracer invariant check: print + abort. */

#define THAPI_ASSERT(cond, fmt, ...)                                                               \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      THAPI_LOG("assertion failed: %s — " fmt, #cond, ##__VA_ARGS__);                              \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)

#define ZE_MUST(call)                                                                              \
  do {                                                                                             \
    ze_result_t _r = (call);                                                                       \
    THAPI_ASSERT(_r == ZE_RESULT_SUCCESS, "%s = 0x%x", #call, _r);                                 \
  } while (0)

static int _do_profile = 0;
static int _do_chained_structs = 0;
static int _do_paranoid_drift = 0;
static int _do_paranoid_memory_location = 0;
static int _do_ddi_table_forward = 0;

/**
 *      _
 *     /  |  _   _     ._ _
 *     \_ | (_) _> |_| | (/_
 *
 */

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

/**
 *     ___
 *      | ._ _.  _ o ._   _
 *      | | (_| (_ | | | (_|
 *                        _|
 */

struct _ze_slot;
struct _ze_slot_slab;
struct _ze_event_h;

/* SLOT / SLAB lifetime — see README.md ("Timestamp profiling architecture")
 * for the ownership graph, counters/state-machine, and teardown diagrams.
 *
 * Dependency-tracking slot: one per profiled Append. Slots carry the
 * happens-before edges the user established (via cl in-order semantics
 * and via phWaitEvents). At sync time we walk these edges from the
 * synced anchor and drain everything reachable. Drain is pop semantics:
 * after emit, the slot is dropped from the cl's list. */
struct _ze_slot {
  struct _ze_slot_slab *slab; /* slab this slot lives in; slab->owner is the cl */
  /* Membership in the cl's live_slots (in-order cls only); the tail is the
   * in-order predecessor a new Append depends on. See live_slots. */
  struct _ze_slot *live_prev, *live_next;
  /* Tracer-owned KERNEL_TIMESTAMP event the kernel/copy signals (the prologue
   * swapped it in for the user's signal). Carries the real kernel timing; drain
   * host-reads it via zeEventQueryKernelTimestamp. Ours — we own its lifecycle
   * (sync/query/reset/dispose); we never touch the user's event. */
  struct _ze_event_h *inj;
  ze_event_handle_t attr; /* user's original signal event (NULL => inj->event) */
  /* User wait events copied at Append time (stable across rebuilds); preds[]
   * is derived from them at instantiate: each wait resolves to the slot that
   * last signaled that event. */
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

#define _ZE_SLAB_SLOTS 64

/* Slot storage in fixed-size slabs; cl_data->slabs is a utlist DL of
 * these. Imm cls allocate new slabs as needed (no cap); regular cls stop
 * at one slab — the inj-signal swap and the user-signal barrier are baked
 * into the closed cl body per Append, so adding a slab after Close would
 * create slots the body doesn't address.
 *
 * The slab frees itself when n_held drops to 0 AND it is not the tail
 * (new Appends still want to land on the tail). */
struct _ze_slot_slab {
  /* The cl these slots live in, shared by every slot in the slab. NULL on a
   * DETACHED slab (see n_pinned). */
  struct _ze_command_list_obj_data *owner;
  uint32_t n_used; /* slots ever assigned in this slab (monotonic until slab free) */
  uint32_t n_held; /* unreleased slots (n_used minus _slot_release calls) */
  /* Nonzero only on a DETACHED slab: one whose owning cl was torn down
   * (reset/destroy) while >=1 slot was still referenced as a pred by a live
   * slot in ANOTHER cl. The slab is removed from cl_data->slabs, its slots'
   * resources are already released and owner==NULL — only the struct survives
   * so the referrers' preds[] pointers stay valid. n_pinned counts those
   * surviving referenced slots; the downstream drain that drops the last ref
   * frees the struct. 0 for normal attached slabs. */
  uint32_t n_pinned;
  struct _ze_slot_slab *next, *prev;
  struct _ze_slot slots[_ZE_SLAB_SLOTS];
};

struct _ze_command_list_obj_data {
  void *ptr;
  UT_hash_handle hh;

  struct _ze_slot_slab *slabs; /* utlist DL_ head; tail = slabs->prev (circular) */

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

  /* Live slots in append order (in-order cls only), utlist DL head via
   * _ze_slot.live_prev/live_next. The tail (live_slots->live_prev) is the most
   * recent still-live slot — the in-order predecessor a new Append depends on,
   * fetched in O(1). Slots join at _slot_instantiate and leave at _slot_drain;
   * bulk teardown (reset/destroy) drops the whole list by nulling this head. */
  struct _ze_slot *live_slots;

  /* Context this cl was created on, stored at create (immutable for the cl's
   * lifetime). Lets the Append prologue and _on_destroy_context's sweep get the
   * cl's context without an L0 roundtrip per Append / per cl. */
  ze_context_handle_t context;

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
 * drain just those cls instead of scanning every live cl (O(live cls) per sync).
 * Buckets are created lazily at Execute and freed when they go empty at drain. */
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
                                           ze_context_handle_t context,
                                           bool immediate,
                                           bool in_order) {
  struct _ze_command_list_obj_data *cl_data =
      (struct _ze_command_list_obj_data *)calloc(1, sizeof(*cl_data));
  if (!cl_data) {
    THAPI_DBGLOG("Failed to allocate memory");
    return;
  }
  cl_data->ptr = (void *)command_list;
  cl_data->context = context;
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

/* An injected event we own. Lives either in its context's freelist (between
 * uses) or anchored to a slot (in flight). The backing L0 pool is shared and
 * owned by the per-context entry, not by this. */
struct _ze_event_h {
  ze_event_handle_t event;
  ze_context_handle_t context;
  struct _ze_event_h *next; /* utlist LL link for the freelist (LIFO) */
};

/* One L0 event pool. zeEventPoolCreate is expensive, so we allocate pools with
 * capacity _ZE_EVENT_POOL_CAP and hand out events by index rather than one pool
 * per event. Kept in the entry's `pools` list so all pools of a context are
 * destroyed together when the context dies. */
#define _ZE_EVENT_POOL_CAP 64
struct _ze_event_pool_node {
  ze_event_pool_handle_t pool;
  struct _ze_event_pool_node *next; /* utlist LL link */
};

/* Per-context injected-event allocator: a freelist of idle events to
 * reuse, plus the pools they were created from. New events fill `cur_pool` by
 * bumping `next_index`; when it reaches _ZE_EVENT_POOL_CAP a fresh pool is
 * created. All events (and pools) live until the context is destroyed. */
struct _ze_event_pool_entry {
  ze_context_handle_t context;
  UT_hash_handle hh;
  struct _ze_event_h *events; /* freelist of reset, ready-to-reuse events */
  struct _ze_event_pool_node *pools;
  ze_event_pool_handle_t cur_pool; /* pool currently being filled (NULL until first event) */
  uint32_t next_index;             /* next free index in cur_pool */
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

/* Find-or-create the entry for ev. NULL only on OOM. Callers pass a non-NULL
 * ev (both reach here under an if (s->attr) gate). */
static struct _ze_event_state_entry *_event_state_get_or_add(ze_event_handle_t ev) {
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
  if (!ev) /* live guard: ev is s->attr, NULL when the user passed no signal */
    return;
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (e && e->latest == slot) {
    e->latest = NULL;
    _event_state_gc(e);
  }
}

// Stash kernel timing for the user's own zeEventQueryKernelTimestamp.
static inline void _event_kts_set(ze_event_handle_t ev, ze_kernel_timestamp_result_t val) {
  struct _ze_event_state_entry *e = _event_state_get_or_add(ev);
  if (e) {
    e->kts = val;
    e->has_kts = true;
  }
}

static inline bool _event_kts_get(ze_event_handle_t ev, ze_kernel_timestamp_result_t *out) {
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (!e || !e->has_kts)
    return false;
  *out = e->kts;
  return true;
}

/* Evict the whole entry (both facts) — called when the event is destroyed. */
static inline void _event_state_del(ze_event_handle_t ev) {
  struct _ze_event_state_entry *e = NULL;
  HASH_FIND_PTR(_ze_event_state, &ev, e);
  if (e) {
    HASH_DEL(_ze_event_state, e);
    free(e);
  }
}

/* Find-or-create the per-context event allocator entry. NULL only on OOM. */
static struct _ze_event_pool_entry *_internal_entry_get_or_add(ze_context_handle_t context) {
  struct _ze_event_pool_entry *entry = NULL;
  HASH_FIND_PTR(_ze_event_pools, &context, entry);
  if (!entry) {
    entry = (struct _ze_event_pool_entry *)calloc(1, sizeof(*entry));
    if (!entry)
      return NULL;
    entry->context = context;
    HASH_ADD_PTR(_ze_event_pools, context, entry);
  }
  return entry;
}

/* Pop one idle event from the per-context freelist; NULL if none cached
 * (caller falls back to creating a fresh L0 event). */
static struct _ze_event_h *_internal_freelist_pop(ze_context_handle_t context) {
  struct _ze_event_pool_entry *entry = NULL;
  HASH_FIND_PTR(_ze_event_pools, &context, entry);
  if (!entry || !entry->events)
    return NULL;
  struct _ze_event_h *e = entry->events;
  LL_DELETE(entry->events, e);
  return e;
}

/* Return an idle event to its per-context freelist. The entry always exists
 * (the event was handed out from it), so this only resets the L0 event and
 * re-queues it — the shared L0 pool lives until the context dies. */
static void _put_event(struct _ze_event_h *val) {
  ZE_MUST(ZE_EVENT_HOST_RESET_PTR(val->event));
  struct _ze_event_pool_entry *entry = NULL;
  HASH_FIND_PTR(_ze_event_pools, &val->context, entry);
  THAPI_ASSERT(entry != NULL, "event %p for context %p was not found", val->event, val->context);
  LL_PREPEND(entry->events, val);
}

/* Ensure entry->cur_pool has a free index, creating a new _ZE_EVENT_POOL_CAP
 * pool (and recording it for teardown) when the current one is full or absent.
 * Returns false on OOM / pool-create failure. */
static bool _internal_pool_ensure_room(struct _ze_event_pool_entry *entry) {
  if (entry->cur_pool && entry->next_index < _ZE_EVENT_POOL_CAP)
    return true;
  struct _ze_event_pool_node *node = (struct _ze_event_pool_node *)calloc(1, sizeof(*node));
  if (!node)
    return false;
  ze_event_pool_desc_t desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL,
      ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE, _ZE_EVENT_POOL_CAP};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(entry->context, &desc, 0, NULL, &node->pool);
  if (res != ZE_RESULT_SUCCESS) {
    /* No pool -> no injected event -> this Append (and likely the rest) runs
     * untraced. Warn once so the profiling loss is visible. Under _ze_state_mutex. */
    static int warned = 0;
    if (!warned) {
      warned = 1;
      THAPI_LOG("warning: zeEventPoolCreate failed (0x%x) for context %p; "
                "Appends will not be timed",
                res, (void *)entry->context);
    }
    free(node);
    return false;
  }
  LL_PREPEND(entry->pools, node);
  entry->cur_pool = node->pool;
  entry->next_index = 0;
  return true;
}

/* Hand out an injected event for a context: reuse one from the freelist, else
 * create a fresh event at the next index of the context's shared pool (creating
 * a new pool only when full). Caller holds the state mutex. */
static struct _ze_event_h *_internal_event_alloc(ze_context_handle_t context) {
  struct _ze_event_h *e = _internal_freelist_pop(context);
  if (e)
    return e;
  struct _ze_event_pool_entry *entry = _internal_entry_get_or_add(context);
  if (!entry || !_internal_pool_ensure_room(entry))
    return NULL;
  e = (struct _ze_event_h *)calloc(1, sizeof(*e));
  if (!e) {
    THAPI_DBGLOG("Could not allocate an event for context: %p", context);
    return NULL;
  }
  ze_event_desc_t e_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, NULL, entry->next_index,
                            ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
  ze_result_t res = ZE_EVENT_CREATE_PTR(entry->cur_pool, &e_desc, &e->event);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventCreate failed with %d, for event pool: %p, context: %p", res,
                 entry->cur_pool, context);
    free(e);
    return NULL;
  }
  entry->next_index++;
  e->context = context;
  return e;
}

/* Append-prologue helper: look up the cl's context (stored at create) and hand
 * out an injected event for it, in one critical section — no per-Append
 * zeCommandListGetContextHandle. Returns NULL (and leaves *ctx NULL) when the cl
 * isn't registered or no event is free; the Append then runs untraced with the
 * user's signal unchanged. */
static struct _ze_event_h *_get_event(ze_command_list_handle_t command_list,
                                      ze_context_handle_t *ctx) {
  struct _ze_event_h *e = NULL;
  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find(command_list);
  if (cl_data) {
    *ctx = cl_data->context;
    e = _internal_event_alloc(cl_data->context);
  }
  pthread_mutex_unlock(&_ze_state_mutex);
  return e;
}

/* Unlink slab c from cl_data->slabs and free the struct. Slot-side cleanup
 * (events, waits, preds) is the caller's responsibility — this helper only
 * owns the slab envelope. */
static void _cl_slab_free(struct _ze_command_list_obj_data *cl_data, struct _ze_slot_slab *c) {
  DL_DELETE(cl_data->slabs, c);
  free(c);
}

/* Allocate a new slab and append it to cl_data->slabs. */
static struct _ze_slot_slab *_cl_slab_alloc(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_slab *c = (struct _ze_slot_slab *)calloc(1, sizeof(*c));
  if (!c)
    return NULL;
  c->owner = cl_data;
  DL_APPEND(cl_data->slabs, c);
  return c;
}

/* Allocate one new slot at the tail of cl_data->slabs. Grows by one
 * slab for imm cls; regular cls stay at one slab and return NULL when
 * full (their inj-signal swap and user-signal barrier are baked into the
 * closed cl body per Append, so a post-Close slab can't be addressed). */
static struct _ze_slot *_cl_slot_append(struct _ze_command_list_obj_data *cl_data,
                                        struct _ze_event_h *inj,
                                        ze_event_handle_t attr,
                                        ze_event_handle_t *waits,
                                        uint32_t n_waits) {
  struct _ze_slot_slab *tail = cl_data->slabs ? cl_data->slabs->prev : NULL;
  if (!tail || tail->n_used >= _ZE_SLAB_SLOTS) {
    if (tail && !cl_data->is_immediate) {
      /* Regular cl is capped at one slab (baked into the closed body). Past
       * the cap we drop the Append's profiling silently — warn once so the
       * data loss is at least visible. Called under _ze_state_mutex. */
      static int warned = 0;
      if (!warned) {
        warned = 1;
        THAPI_LOG("warning: regular command list %p exceeded %d profiled "
                  "Appends in one build; further Appends will not be timed",
                  (void *)cl_data->ptr, _ZE_SLAB_SLOTS);
      }
      return NULL;
    }
    /* The old tail stops being the tail here. If it already drained empty while
     * it was the tail, _slot_release could not free it then (it guards the tail
     * so new Appends keep landing there); this is the one transition that
     * leaves an empty non-tail slab. Reclaim it now — every other empty
     * non-tail slab is freed inline by _slot_release the moment it drains. */
    struct _ze_slot_slab *old_tail = tail;
    tail = _cl_slab_alloc(cl_data);
    if (!tail)
      return NULL;
    if (old_tail && old_tail->n_held == 0)
      _cl_slab_free(cl_data, old_tail);
  }
  uint32_t idx = tail->n_used;
  struct _ze_slot *s = &tail->slots[idx];
  /* Slab memory is calloc'd, so all other slot fields are already zero. */
  s->slab = tail;
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

/* Resolve s->preds: each wait maps to the slot that last signaled it, plus the
 * previous live slot on this cl if the cl is in-order. Marks s live and records
 * it as the latest signaler of its event. */
static void _slot_instantiate(struct _ze_command_list_obj_data *cl_data, struct _ze_slot *s) {
  /* Slot must be inert: live=0, preds NULL. Re-instantiating a live slot would
   * leak its prior pred refs and pick up later-appended live siblings as
   * predecessors, forming cycles that infinite-loop _slot_drain. */
  THAPI_ASSERT(!s->live, "slot %p already live (double _slot_instantiate)", (void *)s);
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
    /* The in-order predecessor is the most recent still-live slot on this cl —
     * the tail of live_slots (list is in append order). O(1); no scan over
     * drained-but-unfreed slots. NULL when this is the first live slot. */
    struct _ze_slot *prev = cl_data->live_slots ? cl_data->live_slots->live_prev : NULL;
    if (prev)
      s->preds[s->n_preds++] = prev;
    DL_APPEND2(cl_data->live_slots, s, live_prev, live_next);
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
static bool _chain_user_signal(ze_command_list_handle_t command_list,
                               ze_event_handle_t inj_event,
                               ze_event_handle_t user_signal,
                               bool in_order) {
  if (!user_signal)
    return false;
  if (in_order)
    ZE_MUST(ZE_COMMAND_LIST_APPEND_SIGNAL_EVENT_PTR(command_list, user_signal));
  else
    ZE_MUST(ZE_COMMAND_LIST_APPEND_BARRIER_PTR(command_list, user_signal, 1, &inj_event));
  return true;
}

/* Roll back the slot just handed out by _cl_slot_append. We were the last to
 * touch the tail slab and hold _ze_state_mutex, so decrementing n_used/n_held
 * and zeroing the slot is safe; if the slab was freshly allocated only for
 * this Append (n_used now 0), free it back so a slot-append failure doesn't
 * leak a slab. */
static void _slot_append_rollback(struct _ze_command_list_obj_data *cl_data, struct _ze_slot *s) {
  free(s->waits);
  struct _ze_slot_slab *c = s->slab;
  c->n_used--;
  c->n_held--;
  memset(s, 0, sizeof(*s));
  if (c->n_used == 0)
    _cl_slab_free(cl_data, c);
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
static void _record_append(ze_command_list_handle_t command_list,
                           ze_context_handle_t ctx,
                           struct _ze_event_h *inj,
                           ze_event_handle_t user_signal,
                           ze_event_handle_t *user_waits,
                           uint32_t user_n_waits) {
  if (!ctx)
    return;
  struct _ze_slot *s = NULL;
  bool signal_chained = false;
  bool in_order = false;

  inj->context = ctx;

  pthread_mutex_lock(&_ze_state_mutex);
  struct _ze_command_list_obj_data *cl_data = _cl_find(command_list);
  if (!cl_data)
    goto fail_locked;
  in_order = cl_data->is_in_order;

  s = _cl_slot_append(cl_data, inj, user_signal, user_waits, user_n_waits);
  if (!s)
    goto fail_locked;

  /* Re-expose the user's signal (the prologue swapped user_signal for inj).
   * In-order: a plain SignalEvent after the kernel; OOO: a barrier waiting on
   * inj. No-op when user_signal is NULL. */
  signal_chained = _chain_user_signal(command_list, inj->event, user_signal, in_order);
  _slot_instantiate(cl_data, s);
  pthread_mutex_unlock(&_ze_state_mutex);
  return;

fail_locked:
  if (s)
    _slot_append_rollback(cl_data, s);
  _put_event(inj);
  pthread_mutex_unlock(&_ze_state_mutex);
  /* Compensate outside the state mutex: if we bailed before chaining
   * user_signal off inj, do it now or the user's Sync(user_signal) hangs. */
  if (!signal_chained)
    _chain_user_signal(command_list, inj->event, user_signal, in_order);
}

/* Dispose the per-slot resources shared by every teardown path: the inj event,
 * the waits[]/preds[] arrays, and the slot's latest-signaler entry. Disposal
 * target differs by caller:
 *   _ZE_DISPOSE_RECYCLE -> _put_event (ctx alive: event recycles to the freelist)
 *   _ZE_DISPOSE_DESTROY -> ctx dying: destroy the L0 event now (L0 requires every
 *                          event be destroyed before its pool, which _on_destroy_context
 *                          destroys next) and free the event struct
 * Nulls every field so it is idempotent; leaves slab/refs/owner/live to the
 * caller. */
enum _ze_slot_dispose_mode { _ZE_DISPOSE_RECYCLE, _ZE_DISPOSE_DESTROY };
static void _slot_dispose_resources(struct _ze_slot *s, enum _ze_slot_dispose_mode mode) {
  if (s->inj) {
    if (mode == _ZE_DISPOSE_DESTROY) {
      if (s->inj->event)
        ZE_MUST(ZE_EVENT_DESTROY_PTR(s->inj->event));
      free(s->inj);
    } else {
      _put_event(s->inj);
    }
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
 * decrement slab n_held; if the slab hits 0 AND isn't the active
 * tail, unlink and free it. Regular cls are skipped (their inj is
 * baked into the cl body — reclaim happens at cl destroy instead). */
static void _slot_release(struct _ze_slot *s) {
  struct _ze_slot_slab *c = s->slab;
  /* Detached slot: its owning cl was torn down (reset/destroy) while this
   * slot was still a pred of a live slot elsewhere. Its resources were freed
   * at reclaim and the slab's owner was nulled; the slab struct was kept
   * alive only to keep this slot's refs addressable. We are the downstream
   * drain dropping the last ref — drop the slab's pin and free it at zero. */
  if (c && !c->owner && c->n_pinned) {
    if (--c->n_pinned == 0)
      free(c);
    return;
  }
  if (!c || !c->owner || !c->owner->is_immediate)
    return;
  /* Reached only from _slot_drain, which already released these; re-running is
   * a harmless no-op. */
  _slot_dispose_resources(s, _ZE_DISPOSE_RECYCLE);

  struct _ze_command_list_obj_data *cl = c->owner;
  c->n_held--;
  if (c->n_held == 0 && c != cl->slabs->prev)
    _cl_slab_free(cl, c);
}

/* Drain one slot. Recurses on its preds, emits the slot's tracepoint,
 * drops one ref on each pred (releasing fully-drained-and-unreferenced
 * preds), then releases s if its own refs hit 0. Safe to call on an
 * already-drained (live=0) slot.
 *
 * No cycle guard: every pred is a slot that became live strictly before us
 * (an earlier slot in the same cl, or the slot that last signaled a wait
 * event), so the edges form a DAG. */
static void _slot_drain(struct _ze_slot *s) {
  if (!s->live)
    return;
  for (uint32_t i = 0; i < s->n_preds; ++i)
    _slot_drain(s->preds[i]);
  s->live = false;
  struct _ze_command_list_obj_data *owner = s->slab ? s->slab->owner : NULL;
  if (owner) {
    owner->n_live--;
    /* Leave the in-order live list (in-order cls only; OOO slots never joined).
     * Detached slabs have owner==NULL — their cl's list head was already
     * dropped at teardown, so there is nothing to unlink from. */
    if (owner->is_in_order)
      DL_DELETE2(owner->live_slots, s, live_prev, live_next);
  }
  ze_event_handle_t attr = s->attr ? s->attr : (s->inj ? s->inj->event : NULL);
  /* Host-read the kernel timing from our injected KERNEL_TIMESTAMP event. Drain
   * runs after a sync that already completed the work, so the query returns
   * immediately; the HostSynchronize is a cheap guard against under-synced user
   * code. */
  bool have_result = false;
  ze_kernel_timestamp_result_t r;
  if (s->inj && s->inj->event) {
    ZE_MUST(ZE_EVENT_HOST_SYNCHRONIZE_PTR(s->inj->event, UINT64_MAX));
    ZE_MUST(ZE_EVENT_QUERY_KERNEL_TIMESTAMP_PTR(s->inj->event, &r));
    have_result = true;
  }
  if (have_result && attr) {
    /* Stash the kernel result under the user's own event so their
     * zeEventQueryKernelTimestamp returns kernel timing, not the op timing their
     * event now carries (we swapped it for inj). Only when they supplied one. */
    if (s->attr)
      _event_kts_set(s->attr, r);
    if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results))
      do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, attr, ZE_RESULT_SUCCESS,
                    ZE_RESULT_SUCCESS, r.global.kernelStart, r.global.kernelEnd,
                    r.context.kernelStart, r.context.kernelEnd);
  }
  /* Reset our inj now that we've read it, so it starts clean for its next
   * Execute. Immediate cls instead reset it when disposed to the pool at
   * _slot_release, so only regular (kept-across-replay) slots reset here. */
  if (s->inj && s->inj->event && owner && !owner->is_immediate)
    ZE_MUST(ZE_EVENT_HOST_RESET_PTR(s->inj->event));
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

/* Drain every live slot in a cl (walk slabs oldest-to-newest, slots
 * low-to-high — natural time order for emission). */
static void _cl_drain(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_slab *c, *tmp;
  DL_FOREACH_SAFE (cl_data->slabs, c, tmp) {
    /* Bump refcount during traversal so the last _slot_drain doesn't
     * free c out from under the inner loop. Drop after, free here. */
    c->n_held++;
    for (uint32_t i = 0; i < c->n_used; ++i)
      _slot_drain(&c->slots[i]);
    c->n_held--;
    if (c->n_held == 0 && c != cl_data->slabs->prev)
      _cl_slab_free(cl_data, c);
  }
  _cl_index_clear(cl_data);
  cl_data->in_flight_q = NULL;
  cl_data->in_flight_fence = NULL;
}

/* Reclaim one slab during cl teardown (reset or single-cl destroy, ctx
 * alive). Releases every slot's resources (events to pool, waits, preds,
 * clears latest-signaled), then either frees the slab or — if any slot is
 * still referenced as a pred by a live slot in ANOTHER cl (refs>0) — DETACHES
 * it: unlink from cl_data->slabs, null each slot's owner, and keep the bare
 * struct alive with n_pinned = #referenced slots. The downstream drains that
 * drop those refs free the struct (see _slot_release's detached branch).
 * Without this, freeing the slab here would dangle the referrers' preds[]. */
static void _cl_slab_reclaim(struct _ze_command_list_obj_data *cl_data, struct _ze_slot_slab *c) {
  uint32_t pinned = 0;
  for (uint32_t i = 0; i < c->n_used; ++i) {
    struct _ze_slot *s = &c->slots[i];
    _slot_dispose_resources(s, _ZE_DISPOSE_RECYCLE);
    if (s->refs)
      pinned++;
  }
  if (pinned == 0) {
    _cl_slab_free(cl_data, c);
    return;
  }
  /* Detach: keep the struct alive for the surviving referenced slots, marked by
   * owner==NULL (drain/release then take the detached path: free at n_pinned==0).
   * The alternative — sever the referrers' preds[] and free the slab now — would
   * need to FIND the referrers, but edges are one-directional (a slot knows its
   * preds[] and a refs COUNT, not WHO points at it). Finding them means an O(all
   * live slots) scan or a per-Append reverse-edge list; both tax the hot path to
   * spare this rare one. Detach keeps refs a mere count: the referrer holds our
   * address in its preds[], so it drops our ref when it drains. */
  DL_DELETE(cl_data->slabs, c);
  c->owner = NULL;
  c->n_pinned = pinned;
}

/* Reclaim (or detach) every slab of a cl whose ctx is still alive, plus clear
 * its in-flight index membership. Shared by reset (keep cl_data) and single-cl
 * destroy (free cl_data after). Detaches any slab with slots still referenced
 * cross-cl; see _cl_slab_reclaim. */
static void _cl_reclaim_slabs(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slot_slab *c, *tmp;
  DL_FOREACH_SAFE (cl_data->slabs, c, tmp)
    _cl_slab_reclaim(cl_data, c);
  _cl_index_clear(cl_data);
  /* The live list pointed into the slabs just freed/detached (detached slots
   * had their owner nulled, so their later drain won't touch it). Drop the head
   * so a reset cl starts clean and no dangling slot is referenced. */
  cl_data->live_slots = NULL;
}

/* Reclaim all of a regular cl's slot state, keeping cl_data registered and
 * empty for reuse. Used by the zeCommandListReset hook. */
static void _cl_data_reset(struct _ze_command_list_obj_data *cl_data) {
  _cl_reclaim_slabs(cl_data);
  cl_data->n_live = 0;
  cl_data->in_flight_q = NULL;
  cl_data->in_flight_fence = NULL;
}

/* Release everything cl_data owns and free cl_data itself. Caller has
 * already removed cl_data from _ze_cls (single-cl: _cl_find_and_del;
 * per-ctx sweep: HASH_DEL inside the iter). ctx alive: reclaim per-slab
 * (detaching slabs whose slots are still referenced cross-cl), exactly like
 * reset. ctx destroy: no slot can outlive the ctx (no detach needed); each
 * slot's inj event is destroyed and its struct freed (_ZE_DISPOSE_DESTROY),
 * so all events are gone before _on_destroy_context destroys the pools. */
static void _cl_data_destroy(struct _ze_command_list_obj_data *cl_data, int ctx_destroy) {
  if (!ctx_destroy) {
    _cl_reclaim_slabs(cl_data);
    free(cl_data);
    return;
  }
  _cl_index_clear(cl_data);
  struct _ze_slot_slab *c, *tmp;
  DL_FOREACH_SAFE (cl_data->slabs, c, tmp) {
    for (uint32_t i = 0; i < c->n_used; ++i)
      _slot_dispose_resources(&c->slots[i], _ZE_DISPOSE_DESTROY);
    _cl_slab_free(cl_data, c);
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
    _cl_data_destroy(cl_data, /*ctx_destroy=*/0);
  pthread_mutex_unlock(&_ze_state_mutex);
}

/* zeCommandListReset epilogue. The L0 spec requires the user to have
 * synchronized before Reset, so our slots are drained — but for a REGULAR cl
 * "drained" is not "reclaimed": _slot_release is a no-op for regular cls
 * (their inj is baked into the cl body, kept for reuse across Executes), so
 * the slots linger. Reset wipes that body, so we must reclaim now; otherwise
 * the stale slots are re-published on the next Execute (massive over-count)
 * and their slabs accumulate (leak). We drain defensively first in case the
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
    if (cl_data->context != hContext)
      continue;
    HASH_DEL(_ze_cls, cl_data);
    _cl_data_destroy(cl_data, /*ctx_destroy=*/1);
  }

  /* 2) Per-ctx event allocator. L0 requires every event be destroyed before its
   * pool, so destroy all freelist events first, then the pools. (In-flight
   * events were already destroyed in sweep 1's _ZE_DISPOSE_DESTROY.) */
  struct _ze_event_pool_entry *pe = NULL;
  HASH_FIND_PTR(_ze_event_pools, &hContext, pe);
  if (pe) {
    HASH_DEL(_ze_event_pools, pe);
    struct _ze_event_h *w, *w_tmp;
    LL_FOREACH_SAFE(pe->events, w, w_tmp) {
      if (w->event)
        ZE_MUST(ZE_EVENT_DESTROY_PTR(w->event));
      free(w);
    }
    struct _ze_event_pool_node *n, *n_tmp;
    LL_FOREACH_SAFE(pe->pools, n, n_tmp) {
      ZE_MUST(ZE_EVENT_POOL_DESTROY_PTR(n->pool));
      free(n);
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
    if (s && s->slab && s->slab->owner) {
      struct _ze_command_list_obj_data *owner = s->slab->owner;
      _slot_drain(s);
      if (!owner->n_live) {
        _cl_index_clear(owner);
        owner->in_flight_q = NULL;
        owner->in_flight_fence = NULL;
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
static bool _on_query_kernel_timestamp(ze_event_handle_t hEvent,
                                       ze_kernel_timestamp_result_t *dstptr) {
  if (!hEvent || !dstptr)
    return false;
  pthread_mutex_lock(&_ze_state_mutex);
  bool found = _event_kts_get(hEvent, dstptr);
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
   *    timing from being clobbered by #N, and serializes the same cl reused
   *    from another thread. Syncing a QUEUE (never a user event) only observes
   *    already-submitted work. */
  if (cl_data->in_flight_q) {
    ZE_MUST(ZE_COMMAND_QUEUE_SYNCHRONIZE_PTR(cl_data->in_flight_q, UINT64_MAX));
    _cl_drain(cl_data);
  }
  /* 2) Re-instantiate each not-yet-live slot into the dep graph. A regular cl
   *    is replayed: its inj-signal swap and user-signal chain are baked in the
   *    closed body and re-fire automatically, so a drained slot just needs its
   *    dep-graph entry rebuilt; a live slot (first Execute after Append) is left
   *    as is. */
  struct _ze_slot_slab *c;
  DL_FOREACH (cl_data->slabs, c) {
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

/**
 *      _                                             ___
 *     | \     ._ _  ._      |   _   _.  _|     ()     |  ._  o _|_
 *     |_/ |_| | | | |_) o   |_ (_) (_| (_| o   (_X   _|_ | | |  |_
 *                   |   /                  /
 */

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
    ze_device_properties_t props = {.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    if (ZE_DEVICE_GET_PROPERTIES_PTR(phSubDevices[j], &props) == ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, subdevice, hDriver, hDevice, phSubDevices[j], &props);
  }
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
      ze_device_properties_t props = {.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
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
  ze_kernel_properties_t kernelProperties = {.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES};
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
      ze_driver_properties_t props = {.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES};
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
    ze_memory_allocation_properties_t memAllocProperties = {
        .stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES};
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
  int verbose = 0;
  char *s = getenv("LTTNG_UST_ZE_LIBZE_LOADER");
  void *handle = dlopen(s ? s : "libze_loader.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
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
