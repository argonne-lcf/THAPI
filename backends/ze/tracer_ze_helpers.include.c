/* Algorithm
 * =========
 *
 * On profiled Append (cl, sig=user_sig, waits=user_waits):
 *   - allocate inj from per-context pool; swap user_sig -> inj
 *   - place a Query (see "QKT placement" below)
 *   - allocate a slot {inj, attr=user_sig, off, waits=copy(user_waits)}
 *   - immediate cl: instantiate(slot) inline
 *
 * instantiate(s):
 *   - s.preds = [latest[w] for w in s.waits if live]
 *                + previous live slot in same cl (if cl is in-order)
 *   - s.live = true; latest[s.attr] = &s
 *
 * On Execute(q, cl) prologue:
 *   - lock cl.mtx
 *   - if cl.in_flight_q: Synchronize(in_flight_q); drain_cl(cl)
 *   - shadow-path slots: re-Append Query on shadow cl
 *     inline-path slots: nothing (Query is baked into cl body)
 *   - instantiate every slot in cl
 *   - cl.in_flight_q = q; unlock
 *
 * On Sync (the synced anchor tells us what to drain):
 *   - Sync(ev):  drain(latest[ev])
 *   - Sync(q):   drain_cl(cl) for every cl whose in_flight_q == q
 *   - Sync(cl):  drain_cl(cl)
 *
 * drain(s):
 *   - for p in s.preds: drain(p)
 *   - shadow-path: host-sync on shadow_done, reset, decrement live_queries
 *   - read slab[s.off], emit tracepoint(s.attr or inj)
 *   - clear latest[s.attr] (if it still points at s)
 *   - clear s.live and s.preds
 *   (Build-time fields inj, attr, off, waits stay so the next Execute
 *    can re-instantiate without re-Appending.)
 *
 * QKT placement
 * =============
 *
 * AppendQueryKernelTimestamps (the device-side timestamp read) lives
 * in one of two places, picked at cl create from the queue group's
 * COMPUTE flag and stored in cl_data->is_compute. Both paths share the
 * slot/drain/dep-graph machinery; they only differ in where the QKT is
 * Appended and how the drain knows it has fired.
 *
 *   INLINE (user cl is on a COMPUTE queue group):
 *
 *     Kernel(sig=inj) ──> QKT(wait=inj, sig=user_signal)   [on user cl]
 *
 *     One Append. user_signal IS the QKT-done edge — any user-level
 *     sync (event/queue/cl) that covers user_signal also covers the
 *     QKT. No tracer fence event, no host-sync at drain. For regular
 *     cls the QKT is baked into the cl body once and re-fires on every
 *     Execute.
 *
 *   SHADOW (user cl is copy-only, or queue group unknown):
 *
 *                       ┌─> Barrier(wait=inj, sig=user_signal) [on user cl]
 *     Kernel(sig=inj) ──┤
 *                       └─> QKT(wait=inj, sig=shadow_done)     [on shadow cl]
 *
 *     Two Appends. The shadow cl is a per-(context, device) tracer-owned
 *     immediate compute cl; QKT goes there because copy queue groups
 *     reject AppendQueryKernelTimestamps. shadow_done is a tracer-owned
 *     fence event that drain host-syncs on — required because the
 *     shadow cl's completion isn't implied by any user-level sync. For
 *     regular cls the shadow QKT is (re-)Appended in the Execute
 *     epilogue (the user cl is in flight by then, so Appending the
 *     Query won't deadlock on a shared engine).
 */

#ifdef THAPI_DEBUG
#define TAHPI_LOG stderr
/* GCC's `, ##__VA_ARGS__` extension swallows the leading comma when the
 * variadic list is empty, so the same macro covers both no-arg and
 * with-args calls. Already used in utils/tracepoint_gen.rb. */
#define THAPI_DBGLOG(fmt, ...)                                                                     \
  do {                                                                                             \
    fprintf(TAHPI_LOG, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);              \
  } while (0)
#else
#define THAPI_DBGLOG(...)                                                                          \
  do {                                                                                             \
  } while (0)
#endif

/* Wrap a tracer-issued L0 call whose failure means we'd either hang the
 * user (sync chain Barrier) or produce a non-self-consistent trace
 * (QKT, event create, ...). Defensive: print + abort so the bug surfaces
 * under sanitizers/CI rather than ship bad data. NOT for driver query
 * calls (Get*Handle, GetCommandQueueGroupProperties) — those can fail
 * transiently during teardown and have graceful fallbacks. */
#define _ZE_MUST(call)                                                                             \
  do {                                                                                             \
    ze_result_t _r = (call);                                                                       \
    if (_r != ZE_RESULT_SUCCESS) {                                                                 \
      fprintf(stderr, "THAPI: tracer-issued L0 call failed: %s = 0x%x at %s:%d\n", #call, _r,      \
              __FILE__, __LINE__);                                                                 \
      abort();                                                                                     \
    }                                                                                              \
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
struct _ze_slab_chunk;

/* Dependency-tracking slot: one per profiled Append. Slots carry the
 * happens-before edges the user established (via cl in-order semantics
 * and via phWaitEvents). At sync time we walk these edges from the
 * synced anchor and drain everything reachable. Drain is pop semantics:
 * after emit, the slot is dropped from the cl's list. */
struct _ze_slot {
  struct _ze_command_list_obj_data *owner; /* cl_data this slot lives in */
  struct _ze_slab_chunk *chunk; /* chunk this slot lives in (==> .slab to read at drain) */
  /* Shadow path only: shadow cl the Query was Appended to. Inline-path
   * slots leave this NULL — their Query lives in the user cl body and
   * the dep-graph walk that triggers drain already implies it has run. */
  struct _ze_shadow_cl *sh;
  struct _ze_event_h *inj; /* tracer-owned event the Query waits on */
  /* Shadow path only: tracer-owned fence event the Query signals; drain
   * host-syncs on it. Inline-path slots leave this NULL. */
  struct _ze_event_h *shadow_done;
  ze_event_handle_t attr; /* user's original signal event (NULL => inj->event) */
  size_t off;             /* byte offset within chunk->slab */
  /* User wait events copied at Append time (stable across rebuilds);
   * preds[] is computed at instantiate from waits[] by looking up
   * latest[w] for each w. */
  ze_event_handle_t *waits;
  uint32_t n_waits;
  struct _ze_slot **preds; /* points at slots whose drain must come first (may be in another cl) */
  uint32_t n_preds;
  unsigned char live; /* in-flight (instantiated, not drained) */
  /* Incoming pred edges: count of downstream slots whose preds[] points
   * here AND that have not yet been drained. Incremented at downstream
   * _slot_instantiate (one per pred edge), decremented at downstream
   * _slot_drain. Slot is reclaimable iff live==0 AND refs==0. Atomic
   * because increment/decrement happen across cl boundaries without
   * holding the slot's owner mtx. */
  uint32_t refs;
};

#define _ZE_SLAB_CHUNK_SLOTS 64

/* Slot + slab storage in fixed-size chunks; cl_data->chunks is a utlist
 * DL of these. Imm cls allocate new chunks as needed (no cap); regular
 * cls stop at one chunk — the inj events (and on the inline path, the
 * QKT itself) are baked into the closed cl body, so adding a chunk
 * after Close would create slots the body doesn't address.
 *
 * Within a chunk, slots[i].off is i * sizeof(timestamp) into slab. The
 * chunk frees itself when n_held drops to 0 AND it is not the tail
 * (new Appends still want to land on the tail). */
struct _ze_slab_chunk {
  void *slab;                   /* _ZE_SLAB_CHUNK_SLOTS * sizeof(ze_kernel_timestamp_result_t) */
  ze_context_handle_t slab_ctx; /* context the slab was allocated against (zeMemFree target) */
  uint32_t n_used;              /* slots ever assigned in this chunk (monotonic until chunk free) */
  uint32_t n_held;              /* unreleased slots (n_used minus _slot_release calls) */
  struct _ze_slab_chunk *next, *prev;
  struct _ze_slot slots[_ZE_SLAB_CHUNK_SLOTS];
};

struct _ze_command_list_obj_data {
  void *ptr;
  UT_hash_handle hh;

  struct _ze_slab_chunk *chunks; /* utlist DL_ head; tail = chunks->prev (circular) */

  /* in_flight_q is the queue this cl was last Executed on AND not yet
   * drained. NULL means "not in flight" — safe to Execute without a
   * force-sync. Set on Execute, cleared on drain.
   *
   * Held only for regular cls; immediate cls never Execute. */
  ze_command_queue_handle_t in_flight_q;
  /* Serializes the Execute prologue: if two threads race to Execute the
   * same closed cl on different queues, we need to force-sync the prior
   * one before letting the second run instantiate. */
  pthread_mutex_t mtx;
  unsigned char is_immediate;
  unsigned char is_in_order;
  /* 1 if this cl's queue group exposes COMPUTE — its body can host
   * AppendQueryKernelTimestamps directly, so we skip the per-(ctx,device)
   * shadow cl and bake QKT into the user cl itself. See the placement
   * diagram at the top of this file. 0 for copy-only cls and for any cl
   * whose group flags we couldn't determine. Set at create; immutable. */
  unsigned char is_compute;

  /* Cached on first use: device handle and context handle for this cl.
   * Both are immutable for the life of the cl, so caching avoids the
   * per-Append/per-Execute ZE_*_GET_*_HANDLE_PTR roundtrips. */
  ze_device_handle_t cached_device;
  ze_context_handle_t cached_context;
};

struct _ze_command_list_obj_data *_ze_cls = NULL;
pthread_mutex_t _ze_cls_mutex = PTHREAD_MUTEX_INITIALIZER;

#define FIND_ZE_CL(key, val)                                                                       \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_cls_mutex);                                                            \
    HASH_FIND_PTR(_ze_cls, key, val);                                                              \
    pthread_mutex_unlock(&_ze_cls_mutex);                                                          \
  } while (0)

#define ADD_ZE_CL(val)                                                                             \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_cls_mutex);                                                            \
    HASH_ADD_PTR(_ze_cls, ptr, val);                                                               \
    pthread_mutex_unlock(&_ze_cls_mutex);                                                          \
  } while (0)

#define FIND_AND_DEL_ZE_CL(key, val)                                                               \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_cls_mutex);                                                            \
    HASH_FIND_PTR(_ze_cls, key, val);                                                              \
    if (val) {                                                                                     \
      HASH_DEL(_ze_cls, val);                                                                      \
    }                                                                                              \
    pthread_mutex_unlock(&_ze_cls_mutex);                                                          \
  } while (0)

/* Per-device cache of the queue-group flag bitmap. The lookup is
 * read-mostly: scan zeDeviceGetCommandQueueGroupProperties once,
 * remember the per-ordinal flags. flags==NULL means "we already checked
 * and the device returned no groups". Used by two readers:
 *   _get_compute_ordinal(dev)        -> first COMPUTE ord, or -1
 *   _ordinal_is_compute(dev, ord)    -> 1 if ord is COMPUTE on dev */
struct _ze_qgroup_cache_entry {
  ze_device_handle_t device;
  ze_command_queue_group_property_flags_t *flags; /* owned; n_groups entries */
  uint32_t n_groups;
  UT_hash_handle hh;
};
static struct _ze_qgroup_cache_entry *_ze_qgroup_cache = NULL;
static pthread_mutex_t _ze_qgroup_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Populate (or return cached) flag bitmap for device. The cache lives
 * for process lifetime — once published, the entry pointer and its flags
 * array are immutable, so callers can dereference them without the
 * mutex. Returns NULL on driver error / OOM. */
static struct _ze_qgroup_cache_entry *_qgroup_cache_get(ze_device_handle_t device) {
  pthread_mutex_lock(&_ze_qgroup_cache_mutex);
  struct _ze_qgroup_cache_entry *e = NULL;
  HASH_FIND_PTR(_ze_qgroup_cache, &device, e);
  pthread_mutex_unlock(&_ze_qgroup_cache_mutex);
  if (e)
    return e;

  /* Slow path: scan queue groups outside the lock. */
  uint32_t n_groups = 0;
  if (ZE_DEVICE_GET_COMMAND_QUEUE_GROUP_PROPERTIES_PTR(device, &n_groups, NULL) !=
          ZE_RESULT_SUCCESS ||
      n_groups == 0)
    return NULL;
  ze_command_queue_group_properties_t *groups =
      (ze_command_queue_group_properties_t *)calloc(n_groups, sizeof(*groups));
  if (!groups)
    return NULL;
  for (uint32_t i = 0; i < n_groups; ++i)
    groups[i].stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
  if (ZE_DEVICE_GET_COMMAND_QUEUE_GROUP_PROPERTIES_PTR(device, &n_groups, groups) !=
      ZE_RESULT_SUCCESS) {
    free(groups);
    return NULL;
  }
  ze_command_queue_group_property_flags_t *flags =
      (ze_command_queue_group_property_flags_t *)calloc(n_groups, sizeof(*flags));
  if (!flags) {
    free(groups);
    return NULL;
  }
  for (uint32_t i = 0; i < n_groups; ++i)
    flags[i] = groups[i].flags;
  free(groups);

  pthread_mutex_lock(&_ze_qgroup_cache_mutex);
  HASH_FIND_PTR(_ze_qgroup_cache, &device, e);
  if (!e) {
    e = (struct _ze_qgroup_cache_entry *)calloc(1, sizeof(*e));
    if (e) {
      e->device = device;
      e->flags = flags;
      e->n_groups = n_groups;
      HASH_ADD_PTR(_ze_qgroup_cache, device, e);
    }
  }
  pthread_mutex_unlock(&_ze_qgroup_cache_mutex);
  if (!e || e->flags != flags)
    free(flags);
  return e;
}

/* Returns the first COMPUTE queue group ordinal for device, or (uint32_t)-1
 * if the device exposes no compute group (fatal — caller should bail). */
static uint32_t _get_compute_ordinal(ze_device_handle_t device) {
  struct _ze_qgroup_cache_entry *e = _qgroup_cache_get(device);
  if (!e)
    return (uint32_t)-1;
  for (uint32_t i = 0; i < e->n_groups; ++i)
    if (e->flags[i] & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE)
      return i;
  return (uint32_t)-1;
}

/* 1 iff `ordinal` on `device` is a COMPUTE queue group. Returns 0 on any
 * uncertainty (unknown device, OOB ordinal, driver error) — callers
 * should treat the cl as non-compute and use the shadow-cl QKT path. */
static int _ordinal_is_compute(ze_device_handle_t device, uint32_t ordinal) {
  if (!device)
    return 0;
  struct _ze_qgroup_cache_entry *e = _qgroup_cache_get(device);
  return e && ordinal < e->n_groups &&
         (e->flags[ordinal] & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) ? 1 : 0;
}

/* Per-(context, device) tracer-owned immediate OOO compute cl used by
 * the SHADOW path to host AppendQueryKernelTimestamps. Copy queue
 * groups reject QKT, so the shadow cl exists to give those user cls
 * somewhere compute-capable to put their Query. Compute user cls take
 * the INLINE path and never touch a shadow cl — see the QKT placement
 * diagram at the top of this file. */
struct _ze_shadow_key {
  ze_context_handle_t context;
  ze_device_handle_t device;
};
struct _ze_shadow_cl {
  struct _ze_shadow_key key;
  ze_command_list_handle_t cl;
  pthread_mutex_t mtx;
  uint32_t live_queries; /* QKTs appended but not yet host-synced; protected by mtx */
  UT_hash_handle hh;
};
static struct _ze_shadow_cl *_ze_shadow_cls = NULL;
static pthread_mutex_t _ze_shadow_cls_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Returns the shadow cl for (context, device), creating it lazily on
 * first use. Returns NULL if the device has no compute group (fatal:
 * we log to stderr) or if creation fails. */
static struct _ze_shadow_cl *_get_shadow_cl(ze_context_handle_t context,
                                            ze_device_handle_t device) {
  struct _ze_shadow_key key = {context, device};
  pthread_mutex_lock(&_ze_shadow_cls_mutex);
  struct _ze_shadow_cl *sh = NULL;
  HASH_FIND(hh, _ze_shadow_cls, &key, sizeof(key), sh);
  if (sh) {
    pthread_mutex_unlock(&_ze_shadow_cls_mutex);
    return sh;
  }
  pthread_mutex_unlock(&_ze_shadow_cls_mutex);

  /* Slow path: create outside the registry lock. */
  uint32_t ord = _get_compute_ordinal(device);
  if (ord == (uint32_t)-1) {
    fprintf(stderr,
            "THAPI: device %p has no COMPUTE queue group; "
            "cannot create shadow cl. Profiling disabled for "
            "command lists on this device.\n",
            (void *)device);
    return NULL;
  }
  /* ASYNCHRONOUS mode is critical: with SYNCHRONOUS (the DEFAULT),
   * each AppendQueryKernelTimestamps on this immediate cl blocks until
   * the Query completes — which it can't, because Query is waiting on
   * inj, and inj is signaled by the user cl's kernel that hasn't been
   * submitted yet (we're called from the user's Execute prologue).
   * Deadlock. ASYNCHRONOUS lets the Append return immediately and the
   * Query run device-side at its own pace. */
  ze_command_queue_desc_t qd = {
      ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC, NULL, ord, 0, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL};
  ze_command_list_handle_t new_cl = NULL;
  if (ZE_COMMAND_LIST_CREATE_IMMEDIATE_PTR(context, device, &qd, &new_cl) != ZE_RESULT_SUCCESS ||
      !new_cl) {
    fprintf(stderr,
            "THAPI: failed to create shadow cl for "
            "context=%p device=%p\n",
            (void *)context, (void *)device);
    return NULL;
  }

  pthread_mutex_lock(&_ze_shadow_cls_mutex);
  HASH_FIND(hh, _ze_shadow_cls, &key, sizeof(key), sh);
  if (sh) {
    /* Lost the race; destroy ours and return the winner. */
    pthread_mutex_unlock(&_ze_shadow_cls_mutex);
    ZE_COMMAND_LIST_DESTROY_PTR(new_cl);
    return sh;
  }
  sh = (struct _ze_shadow_cl *)calloc(1, sizeof(*sh));
  if (!sh) {
    pthread_mutex_unlock(&_ze_shadow_cls_mutex);
    ZE_COMMAND_LIST_DESTROY_PTR(new_cl);
    return NULL;
  }
  sh->key = key;
  sh->cl = new_cl;
  pthread_mutex_init(&sh->mtx, NULL);
  HASH_ADD(hh, _ze_shadow_cls, key, sizeof(sh->key), sh);
  pthread_mutex_unlock(&_ze_shadow_cls_mutex);
  return sh;
}

/* Append AppendQueryKernelTimestamps on the shadow cl: wait on inj,
 * signal shadow_done, write timestamps into slab[*off]. Serialized on
 * sh->mtx because L0 doesn't allow concurrent Appends to one cl.
 * Aborts on L0 failure (defensive — a missing Query would silently
 * drop this kernel's timing). */
static void _shadow_append_query(struct _ze_shadow_cl *sh,
                                 ze_event_handle_t inj_event,
                                 void *slab,
                                 size_t *off,
                                 ze_event_handle_t shadow_done_event) {
  pthread_mutex_lock(&sh->mtx);
  sh->live_queries++;
  _ZE_MUST(ZE_COMMAND_LIST_APPEND_QUERY_KERNEL_TIMESTAMPS_PTR(sh->cl, 1, &inj_event, slab, off,
                                                              /*hSignalEvent=*/shadow_done_event,
                                                              /*numWaitEvents=*/1, &inj_event));
  pthread_mutex_unlock(&sh->mtx);
}

/* Return cl_data->cached_{device,context}, fetching from L0 on first call.
 * Both fields are immutable for the cl's lifetime, so caching avoids the
 * roundtrip on every Append/Execute. Returns NULL on L0 error. */
static ze_device_handle_t _cl_cache_device(struct _ze_command_list_obj_data *cl_data,
                                           ze_command_list_handle_t command_list) {
  if (cl_data->cached_device)
    return cl_data->cached_device;
  ze_device_handle_t d = NULL;
  if (ZE_COMMAND_LIST_GET_DEVICE_HANDLE_PTR(command_list, &d) == ZE_RESULT_SUCCESS)
    cl_data->cached_device = d;
  return d;
}

static ze_context_handle_t _cl_cache_context(struct _ze_command_list_obj_data *cl_data,
                                             ze_command_list_handle_t command_list) {
  if (cl_data->cached_context)
    return cl_data->cached_context;
  ze_context_handle_t c = NULL;
  if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &c) == ZE_RESULT_SUCCESS)
    cl_data->cached_context = c;
  return c;
}

static inline void _on_create_command_list(ze_command_list_handle_t command_list,
                                           ze_device_handle_t device,
                                           uint32_t ordinal, int immediate, int in_order) {
  struct _ze_command_list_obj_data *cl_data = NULL;

  FIND_ZE_CL(&command_list, cl_data);
  if (cl_data) {
    THAPI_DBGLOG("Command list already registered: %p", command_list);
    return;
  }

  cl_data = (struct _ze_command_list_obj_data *)calloc(1, sizeof(*cl_data));
  if (!cl_data) {
    THAPI_DBGLOG("Failed to allocate memory");
    return;
  }
  cl_data->ptr = (void *)command_list;
  cl_data->is_immediate = immediate ? 1 : 0;
  cl_data->is_in_order = in_order ? 1 : 0;
  cl_data->is_compute = _ordinal_is_compute(device, ordinal) ? 1 : 0;
  cl_data->cached_device = device;
  pthread_mutex_init(&cl_data->mtx, NULL);
  ADD_ZE_CL(cl_data);
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
static pthread_mutex_t _ze_event_pools_mutex = PTHREAD_MUTEX_INITIALIZER;

/* latest[ev] -> the most recent slot whose attr==ev. Used to resolve
 * happens-before edges: when a new Append says "wait on ev", we record
 * the latest slot for ev as a pred. Updated at instantiate and cleared
 * at drain. */
struct _ze_latest_entry {
  ze_event_handle_t ev; /* key */
  struct _ze_slot *slot;
  UT_hash_handle hh;
};
static struct _ze_latest_entry *_ze_latest = NULL;
static pthread_mutex_t _ze_latest_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline struct _ze_slot *_latest_get(ze_event_handle_t ev) {
  struct _ze_latest_entry *e = NULL;
  pthread_mutex_lock(&_ze_latest_mutex);
  HASH_FIND_PTR(_ze_latest, &ev, e);
  struct _ze_slot *s = e ? e->slot : NULL;
  pthread_mutex_unlock(&_ze_latest_mutex);
  return s;
}

static inline void _latest_set(ze_event_handle_t ev, struct _ze_slot *s) {
  if (!ev)
    return;
  pthread_mutex_lock(&_ze_latest_mutex);
  struct _ze_latest_entry *e = NULL;
  HASH_FIND_PTR(_ze_latest, &ev, e);
  if (!e) {
    e = (struct _ze_latest_entry *)calloc(1, sizeof(*e));
    if (!e) {
      pthread_mutex_unlock(&_ze_latest_mutex);
      return;
    }
    e->ev = ev;
    HASH_ADD_PTR(_ze_latest, ev, e);
  }
  e->slot = s;
  pthread_mutex_unlock(&_ze_latest_mutex);
}

/* Remove latest[ev] only if it still points at slot s (the slot is
 * being drained — but if a newer Append already overwrote latest[ev],
 * don't clobber that). */
static inline void _latest_clear_if(ze_event_handle_t ev, struct _ze_slot *s) {
  if (!ev)
    return;
  pthread_mutex_lock(&_ze_latest_mutex);
  struct _ze_latest_entry *e = NULL;
  HASH_FIND_PTR(_ze_latest, &ev, e);
  if (e && e->slot == s) {
    HASH_DEL(_ze_latest, e);
    free(e);
  }
  pthread_mutex_unlock(&_ze_latest_mutex);
}

#define GET_ZE_EVENT(key, val)                                                                     \
  do {                                                                                             \
    struct _ze_event_pool_entry *pool = NULL;                                                      \
    pthread_mutex_lock(&_ze_event_pools_mutex);                                                    \
    HASH_FIND_PTR(_ze_event_pools, key, pool);                                                     \
    if (pool && pool->events) {                                                                    \
      val = pool->events;                                                                          \
      DL_DELETE(pool->events, val);                                                                \
    } else                                                                                         \
      val = NULL;                                                                                  \
    pthread_mutex_unlock(&_ze_event_pools_mutex);                                                  \
  } while (0)

#define PUT_ZE_EVENT(val)                                                                          \
  do {                                                                                             \
    struct _ze_event_pool_entry *pool = NULL;                                                      \
    pthread_mutex_lock(&_ze_event_pools_mutex);                                                    \
    HASH_FIND_PTR(_ze_event_pools, &(val->context), pool);                                         \
    if (!pool) {                                                                                   \
      pool = (struct _ze_event_pool_entry *)calloc(1, sizeof(struct _ze_event_pool_entry));        \
      if (!pool) {                                                                                 \
        THAPI_DBGLOG("Failed to allocate memory");                                                 \
        pthread_mutex_unlock(&_ze_event_pools_mutex);                                              \
        if (val->event_pool) {                                                                     \
          if (val->event)                                                                          \
            ZE_EVENT_DESTROY_PTR(val->event);                                                      \
          ZE_EVENT_POOL_DESTROY_PTR(val->event_pool);                                              \
        }                                                                                          \
        free(val);                                                                                 \
        break;                                                                                     \
      }                                                                                            \
      pool->context = val->context;                                                                \
      HASH_ADD_PTR(_ze_event_pools, context, pool);                                                \
    }                                                                                              \
    _ZE_MUST(ZE_EVENT_HOST_RESET_PTR(val->event));                                                 \
    DL_PREPEND(pool->events, val);                                                                 \
    pthread_mutex_unlock(&_ze_event_pools_mutex);                                                  \
  } while (0)

struct _ze_event_h *_ze_event_wrappers = NULL;
static pthread_mutex_t _ze_event_wrappers_mutex = PTHREAD_MUTEX_INITIALIZER;

#define GET_ZE_EVENT_WRAPPER(val)                                                                  \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_event_wrappers_mutex);                                                 \
    if (_ze_event_wrappers) {                                                                      \
      val = _ze_event_wrappers;                                                                    \
      DL_DELETE(_ze_event_wrappers, val);                                                          \
    } else {                                                                                       \
      val = calloc(1, sizeof(struct _ze_event_h));                                                 \
    }                                                                                              \
    pthread_mutex_unlock(&_ze_event_wrappers_mutex);                                               \
  } while (0)

#define PUT_ZE_EVENT_WRAPPER(val)                                                                  \
  do {                                                                                             \
    memset(val, 0, sizeof(struct _ze_event_h));                                                    \
    pthread_mutex_lock(&_ze_event_wrappers_mutex);                                                 \
    DL_PREPEND(_ze_event_wrappers, val);                                                           \
    pthread_mutex_unlock(&_ze_event_wrappers_mutex);                                               \
  } while (0)

static struct _ze_event_h *_get_profiling_event(ze_command_list_handle_t command_list) {
  struct _ze_event_h *e_w;

  ze_context_handle_t context = NULL;
  ze_result_t res = ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &context);
  if (res != ZE_RESULT_SUCCESS || !context) {
    THAPI_DBGLOG("zeCommandListGetContextHandle failed with %d, for command list: %p", res,
                 command_list);
    return NULL;
  }
  GET_ZE_EVENT(&context, e_w);
  if (e_w)
    return e_w;

  GET_ZE_EVENT_WRAPPER(e_w);
  if (!e_w) {
    THAPI_DBGLOG("Could not create a new event wrapper for command list: %p", command_list);
    return NULL;
  }

  ze_event_pool_desc_t desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL,
      ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
  res = ZE_EVENT_POOL_CREATE_PTR(context, &desc, 0, NULL, &e_w->event_pool);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventPoolCreate failed with %d, for command list: %p, context: %p", res,
                 command_list, context);
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
  PUT_ZE_EVENT_WRAPPER(e_w);
  return NULL;
}

/* Allocate a new chunk and append it to cl_data->chunks. */
static struct _ze_slab_chunk *_cl_chunk_alloc(struct _ze_command_list_obj_data *cl_data,
                                              ze_context_handle_t ctx) {
  struct _ze_slab_chunk *c = (struct _ze_slab_chunk *)calloc(1, sizeof(*c));
  if (!c)
    return NULL;
  size_t bytes = (size_t)_ZE_SLAB_CHUNK_SLOTS * sizeof(ze_kernel_timestamp_result_t);
  ze_host_mem_alloc_desc_t hd = {ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, NULL, 0};
  if (ZE_MEM_ALLOC_HOST_PTR(ctx, &hd, bytes, sizeof(uint64_t), &c->slab) != ZE_RESULT_SUCCESS ||
      !c->slab) {
    free(c);
    return NULL;
  }
  memset(c->slab, 0, bytes);
  c->slab_ctx = ctx;
  DL_APPEND(cl_data->chunks, c);
  return c;
}

/* Allocate one new slot at the tail of cl_data->chunks. Grows by one
 * chunk for imm cls; regular cls stay at one chunk and return NULL when
 * full (their inj events are baked into the closed cl body, so storage
 * must keep addressing them via the same (slab, off) pair). */
static struct _ze_slot *_cl_slot_append(struct _ze_command_list_obj_data *cl_data,
                                        ze_context_handle_t ctx,
                                        struct _ze_event_h *inj,
                                        struct _ze_event_h *shadow_done,
                                        ze_event_handle_t attr,
                                        ze_event_handle_t *waits,
                                        uint32_t n_waits) {
  struct _ze_slab_chunk *tail = cl_data->chunks ? cl_data->chunks->prev : NULL;
  if (!tail || tail->n_used >= _ZE_SLAB_CHUNK_SLOTS) {
    if (tail && !cl_data->is_immediate)
      return NULL;
    tail = _cl_chunk_alloc(cl_data, ctx);
    if (!tail)
      return NULL;
  }
  uint32_t idx = tail->n_used;
  struct _ze_slot *s = &tail->slots[idx];
  /* Chunk memory is calloc'd, so all other slot fields are already zero. */
  s->owner = cl_data;
  s->chunk = tail;
  s->inj = inj;
  s->shadow_done = shadow_done;
  s->attr = attr;
  s->off = (size_t)idx * sizeof(ze_kernel_timestamp_result_t);
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

/* Compute s->preds from s->waits via the global latest[] map, plus the
 * previous live slot on this cl if the cl is in-order. Marks s live and
 * publishes s as the new latest[attr]. */
static void _slot_instantiate(struct _ze_command_list_obj_data *cl_data, struct _ze_slot *s) {
  s->live = 1;
  uint32_t cap = s->n_waits + 1; /* +1 for in-order prev */
  s->preds = (struct _ze_slot **)calloc(cap, sizeof(struct _ze_slot *));
  s->n_preds = 0;
  for (uint32_t i = 0; i < s->n_waits; ++i) {
    struct _ze_slot *p = _latest_get(s->waits[i]);
    if (p && p->live)
      s->preds[s->n_preds++] = p;
  }
  if (cl_data->is_in_order) {
    /* Walk chunks newest-to-oldest, slots high-to-low, stop at the first
     * live slot strictly before s. Chunks are appended in time order
     * (DL_APPEND) and slots within a chunk in time order, so reverse-walk
     * yields reverse time order. Skip s itself; s might still have
     * live=0 here but the !=s guard is safe and clearer. */
    struct _ze_slab_chunk *c;
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
    __atomic_fetch_add(&s->preds[i]->refs, 1, __ATOMIC_RELAXED);
  if (s->attr)
    _latest_set(s->attr, s);
}

/* Append-time hook called from profiling_epilogue. Caller already
 * swapped user's hSignalEvent for inj->event. user_signal is the
 * ORIGINAL value (possibly NULL). user_waits is the user's wait list
 * (NULL,0 if none).
 *
 * Forks on cl_data->is_compute to pick the QKT placement (INLINE vs
 * SHADOW) — see the "QKT placement" diagram at the top of this file. */
static void _universal_record_append(ze_command_list_handle_t command_list,
                                     struct _ze_event_h *inj,
                                     ze_event_handle_t user_signal,
                                     ze_event_handle_t *user_waits,
                                     uint32_t user_n_waits) {
  if (!inj)
    return;
  struct _ze_event_h *shadow_done = NULL;
  struct _ze_command_list_obj_data *cl_data = NULL;
  struct _ze_slot *s = NULL;
  int inline_path = 0;
  int barrier_chained = 0;

  ze_context_handle_t ctx = NULL;
  if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &ctx) != ZE_RESULT_SUCCESS || !ctx)
    goto fail;
  inj->context = ctx;

  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data)
    goto fail;
  inline_path = cl_data->is_compute;

  /* Shadow path needs a fence event (Query lives on the shadow cl;
   * drain host-syncs on it). Inline path uses user_signal as the fence
   * via the dep graph, no extra event needed. */
  if (!inline_path) {
    shadow_done = _get_profiling_event(command_list);
    if (!shadow_done)
      goto fail_with_cl;
    shadow_done->context = ctx;
  }

  pthread_mutex_lock(&cl_data->mtx);

  s = _cl_slot_append(cl_data, ctx, inj, shadow_done, user_signal, user_waits, user_n_waits);
  if (!s)
    goto fail_locked;

  if (inline_path) {
    /* Bake the QKT into the user cl. wait=inj, sig=user_signal.
     * Holds for both immediate (fires when Appended) and regular cls
     * (fires on every Execute — the QKT is now part of the cl body). */
    cl_data->cached_context = ctx;
    ze_event_handle_t wait_ev = inj->event;
    _ZE_MUST(ZE_COMMAND_LIST_APPEND_QUERY_KERNEL_TIMESTAMPS_PTR(command_list, 1, &wait_ev,
                                                                s->chunk->slab, &s->off,
                                                                user_signal, 1, &wait_ev));
    barrier_chained = 1; /* user_signal chained via the QKT itself */
    _slot_instantiate(cl_data, s);
    pthread_mutex_unlock(&cl_data->mtx);
    ADD_ZE_CL(cl_data);
    return;
  }

  /* Shadow path: chain user_signal off inj on the user cl, then place
   * the Query on the shadow cl (immediate cls only — regular cls defer
   * to the Execute epilogue, see _on_execute_one_cl). */
  if (user_signal) {
    ze_event_handle_t wait_ev = inj->event;
    _ZE_MUST(ZE_COMMAND_LIST_APPEND_BARRIER_PTR(command_list, user_signal, 1, &wait_ev));
    barrier_chained = 1;
  }
  if (cl_data->is_immediate) {
    cl_data->cached_context = ctx;
    ze_device_handle_t dev = _cl_cache_device(cl_data, command_list);
    struct _ze_shadow_cl *sh = dev ? _get_shadow_cl(ctx, dev) : NULL;
    if (!sh)
      goto fail_locked;
    _shadow_append_query(sh, inj->event, s->chunk->slab, &s->off, shadow_done->event);
    s->sh = sh;
    _slot_instantiate(cl_data, s);
  }
  pthread_mutex_unlock(&cl_data->mtx);
  ADD_ZE_CL(cl_data);
  return;

fail_locked:
  if (s) {
    /* Roll back the slot we just appended. We were the very last to
     * touch the tail chunk and we hold cl_data->mtx, so decrementing
     * n_used/n_held and clearing the slot is safe. If the chunk
     * was freshly allocated only for this Append (n_used now 0), free
     * it back so we don't leak a chunk per slot-append failure. */
    free(s->waits);
    struct _ze_slab_chunk *c = s->chunk;
    c->n_used--;
    c->n_held--;
    memset(s, 0, sizeof(*s));
    if (c->n_used == 0) {
      DL_DELETE(cl_data->chunks, c);
      if (c->slab)
        ZE_MEM_FREE_PTR(c->slab_ctx, c->slab);
      free(c);
    }
  }
  pthread_mutex_unlock(&cl_data->mtx);
fail_with_cl:
  ADD_ZE_CL(cl_data);
fail:
  /* If we never chained user_signal off inj, do it now. The prologue
   * swapped user's sig for inj->event; without this Append the user's
   * Sync(user_signal) would hang forever. Aborts on failure — we have
   * no second-chance recovery and a silent hang is worse than a crash. */
  if (user_signal && !barrier_chained) {
    ze_event_handle_t wait_ev = inj->event;
    _ZE_MUST(ZE_COMMAND_LIST_APPEND_BARRIER_PTR(command_list, user_signal, 1, &wait_ev));
  }
  if (shadow_done)
    PUT_ZE_EVENT(shadow_done);
  PUT_ZE_EVENT(inj);
}

/* Reclaim a slot: PUT events back to the per-context pool, free waits,
 * decrement chunk n_held; if the chunk hits 0 AND isn't the active
 * tail, unlink and free it. Regular cls are skipped (their inj is
 * baked into the cl body — reclaim happens at cl destroy instead). */
static void _slot_release(struct _ze_slot *s) {
  if (!s || !s->owner || !s->owner->is_immediate)
    return;
  if (s->inj) {
    PUT_ZE_EVENT(s->inj);
    s->inj = NULL;
  }
  if (s->shadow_done) {
    PUT_ZE_EVENT(s->shadow_done);
    s->shadow_done = NULL;
  }
  free(s->waits);
  s->waits = NULL;
  s->n_waits = 0;
  s->attr = NULL;

  struct _ze_slab_chunk *c = s->chunk;
  struct _ze_command_list_obj_data *cl = s->owner;
  if (!c)
    return;
  c->n_held--;
  if (c->n_held == 0 && c != cl->chunks->prev) {
    DL_DELETE(cl->chunks, c);
    if (c->slab)
      ZE_MEM_FREE_PTR(c->slab_ctx, c->slab);
    free(c);
  }
}

/* Drain one slot. Recurses on its preds, emits the slot's tracepoint,
 * drops one ref on each pred (releasing fully-drained-and-unreferenced
 * preds), then releases s if its own refs hit 0. Safe to call on an
 * already-drained (live=0) slot.
 *
 * Slab read uses s->chunk->slab — preds may live in another cl, so we
 * can't use the caller's slab.
 *
 * No cycle guard: preds come from in-order prev (strictly earlier slot
 * in the same cl, DAG) and from latest[wait_event] (a slot published
 * BEFORE us). Forming a cycle would require user-declared mutual waits,
 * which L0 itself deadlocks on. */
static void _slot_drain(struct _ze_slot *s) {
  if (!s || !s->live)
    return;
  for (uint32_t i = 0; i < s->n_preds; ++i)
    _slot_drain(s->preds[i]);
  s->live = 0;
  /* Shadow-path only: block until the Query has fired, then reset
   * shadow_done so the next Execute round starts with a clean event.
   * The user's own sync doesn't cover the Query because it runs on the
   * shadow cl. Inline-path slots have shadow_done==NULL — their QKT
   * lives in the user cl body and the dep-graph walk that brought us
   * here already implies it has run. */
  if (s->shadow_done && s->shadow_done->event) {
    _ZE_MUST(ZE_EVENT_HOST_SYNCHRONIZE_PTR(s->shadow_done->event, UINT64_MAX));
    _ZE_MUST(ZE_EVENT_HOST_RESET_PTR(s->shadow_done->event));
    /* QKT completed device-side. Drop the live ref; if nothing else on
     * this shadow cl is in flight, Reset it: the L0 driver leaks ~10 KB
     * per AppendQueryKernelTimestamps and only reclaims at Reset/Destroy. */
    if (s->sh) {
      pthread_mutex_lock(&s->sh->mtx);
      s->sh->live_queries--;
      if (s->sh->live_queries == 0)
        _ZE_MUST(ZE_COMMAND_LIST_RESET_PTR(s->sh->cl));
      pthread_mutex_unlock(&s->sh->mtx);
    }
  }
  ze_event_handle_t attr = s->attr ? s->attr : (s->inj ? s->inj->event : NULL);
  if (s->chunk && s->chunk->slab && attr &&
      tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results)) {
    ze_kernel_timestamp_result_t r =
        *(ze_kernel_timestamp_result_t *)((char *)s->chunk->slab + s->off);
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, attr, ZE_RESULT_SUCCESS,
                  ZE_RESULT_SUCCESS, r.global.kernelStart, r.global.kernelEnd,
                  r.context.kernelStart, r.context.kernelEnd);
  }
  _latest_clear_if(s->attr, s);
  /* Drop refs on preds; release any that hit 0 and are already drained. */
  for (uint32_t i = 0; i < s->n_preds; ++i) {
    struct _ze_slot *p = s->preds[i];
    if (__atomic_sub_fetch(&p->refs, 1, __ATOMIC_RELAXED) == 0 && !p->live)
      _slot_release(p);
  }
  free(s->preds);
  s->preds = NULL;
  s->n_preds = 0;
  if (__atomic_load_n(&s->refs, __ATOMIC_RELAXED) == 0)
    _slot_release(s);
}

/* Drain every live slot in a cl (walk chunks oldest-to-newest, slots
 * low-to-high — natural time order for emission). */
static void _cl_drain(struct _ze_command_list_obj_data *cl_data) {
  struct _ze_slab_chunk *c, *tmp;
  DL_FOREACH_SAFE(cl_data->chunks, c, tmp) {
    /* Bump refcount during traversal so the last _slot_drain doesn't
     * free c out from under the inner loop. Drop after, free here. */
    c->n_held++;
    for (uint32_t i = 0; i < c->n_used; ++i)
      _slot_drain(&c->slots[i]);
    c->n_held--;
    if (c->n_held == 0 && c != cl_data->chunks->prev) {
      DL_DELETE(cl_data->chunks, c);
      if (c->slab)
        ZE_MEM_FREE_PTR(c->slab_ctx, c->slab);
      free(c);
    }
  }
  cl_data->in_flight_q = NULL;
}

/* Drain a single cl. */
static void _on_sync_drain_cl(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_ZE_CL(&command_list, cl_data);
  if (!cl_data)
    return;
  pthread_mutex_lock(&cl_data->mtx);
  _cl_drain(cl_data);
  pthread_mutex_unlock(&cl_data->mtx);
}

/* zeCommandListDestroy epilogue. The L0 spec says the user must have
 * ensured the device is no longer referencing the cl, so we don't drain
 * (the GPU is already idle on this cl). We just release our state:
 * PUT every slot's tracer-owned events back to the per-context pool,
 * free per-slot allocations, free every chunk's slab + chunk struct,
 * remove cl_data from the registry, free cl_data itself.
 *
 * Works for both cl kinds: regular cls (inj baked into the cl body)
 * can recycle inj here because the cl body is about to be destroyed by
 * L0; immediate cls' slots have likely already been released at drain
 * time but any stragglers get cleaned up too. */
static void _on_destroy_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data)
    return;
  pthread_mutex_lock(&cl_data->mtx);
  struct _ze_slab_chunk *c, *tmp;
  DL_FOREACH_SAFE(cl_data->chunks, c, tmp) {
    for (uint32_t i = 0; i < c->n_used; ++i) {
      struct _ze_slot *s = &c->slots[i];
      if (s->inj)
        PUT_ZE_EVENT(s->inj);
      if (s->shadow_done)
        PUT_ZE_EVENT(s->shadow_done);
      free(s->waits);
      free(s->preds);
      _latest_clear_if(s->attr, s);
    }
    DL_DELETE(cl_data->chunks, c);
    if (c->slab)
      ZE_MEM_FREE_PTR(c->slab_ctx, c->slab);
    free(c);
  }
  pthread_mutex_unlock(&cl_data->mtx);
  pthread_mutex_destroy(&cl_data->mtx);
  free(cl_data);
}

/* zeContextDestroy prologue. The user contract is that the device is no
 * longer referencing the context, so all cls/events bound to it are
 * conceptually dead from the user's perspective. Our job here is solely
 * to avoid leaking our own L0 objects that live inside this context:
 *
 *   1) cls registered against this ctx: free their slot/slab/chunk state
 *      (drop tracer-owned events to L0 without re-pooling — the pool is
 *      about to die anyway).
 *   2) per-(ctx, device) shadow cls: zeCommandListDestroy them.
 *   3) per-ctx event-pool freelist: zeEventDestroy + zeEventPoolDestroy
 *      each wrapper, recycle the wrapper structs.
 *
 * Forwards no calls about the user's own cls/events to the driver — the
 * user takes care of those (or accepts the contract). */
static void _on_destroy_context(ze_context_handle_t hContext) {
  /* 1) Drop cls bound to this ctx. */
  pthread_mutex_lock(&_ze_cls_mutex);
  struct _ze_command_list_obj_data *cl_data = NULL, *cl_tmp = NULL;
  HASH_ITER(hh, _ze_cls, cl_data, cl_tmp) {
    if (cl_data->cached_context != hContext)
      continue;
    HASH_DEL(_ze_cls, cl_data);
    pthread_mutex_lock(&cl_data->mtx);
    struct _ze_slab_chunk *c, *ctmp;
    DL_FOREACH_SAFE(cl_data->chunks, c, ctmp) {
      for (uint32_t i = 0; i < c->n_used; ++i) {
        struct _ze_slot *s = &c->slots[i];
        /* Recycle our event wrappers but DON'T return them to the per-ctx
         * pool — the pool entry will be wiped in step 3, and we want the
         * underlying L0 event/pool destroyed there too, not here. The
         * wrappers themselves are context-agnostic, so reuse them. */
        if (s->inj)
          PUT_ZE_EVENT_WRAPPER(s->inj);
        if (s->shadow_done)
          PUT_ZE_EVENT_WRAPPER(s->shadow_done);
        free(s->waits);
        free(s->preds);
        _latest_clear_if(s->attr, s);
      }
      DL_DELETE(cl_data->chunks, c);
      /* Skip zeMemFree on the slab — the ctx is being destroyed; the
       * driver will reclaim the device allocation. Calling zeMemFree
       * on a doomed ctx is at best racy. */
      free(c);
    }
    pthread_mutex_unlock(&cl_data->mtx);
    pthread_mutex_destroy(&cl_data->mtx);
    free(cl_data);
  }
  pthread_mutex_unlock(&_ze_cls_mutex);

  /* 2) Shadow cls keyed by (ctx, device). */
  pthread_mutex_lock(&_ze_shadow_cls_mutex);
  struct _ze_shadow_cl *sh = NULL, *sh_tmp = NULL;
  HASH_ITER(hh, _ze_shadow_cls, sh, sh_tmp) {
    if (sh->key.context != hContext)
      continue;
    HASH_DEL(_ze_shadow_cls, sh);
    if (sh->cl)
      ZE_COMMAND_LIST_DESTROY_PTR(sh->cl);
    pthread_mutex_destroy(&sh->mtx);
    free(sh);
  }
  pthread_mutex_unlock(&_ze_shadow_cls_mutex);

  /* 3) Per-ctx event pool freelist. */
  pthread_mutex_lock(&_ze_event_pools_mutex);
  struct _ze_event_pool_entry *pe = NULL;
  HASH_FIND_PTR(_ze_event_pools, &hContext, pe);
  if (pe) {
    HASH_DEL(_ze_event_pools, pe);
    struct _ze_event_h *w, *w_tmp;
    DL_FOREACH_SAFE(pe->events, w, w_tmp) {
      if (w->event)
        ZE_EVENT_DESTROY_PTR(w->event);
      if (w->event_pool)
        ZE_EVENT_POOL_DESTROY_PTR(w->event_pool);
      DL_DELETE(pe->events, w);
      PUT_ZE_EVENT_WRAPPER(w);
    }
    free(pe);
  }
  pthread_mutex_unlock(&_ze_event_pools_mutex);
}

/* Drain every cl whose in_flight_q matches. */
static void _on_sync_drain_queue(ze_command_queue_handle_t hQueue) {
  pthread_mutex_lock(&_ze_cls_mutex);
  struct _ze_command_list_obj_data *cl_data = NULL, *tmp = NULL;
  HASH_ITER(hh, _ze_cls, cl_data, tmp) {
    if (cl_data->in_flight_q == hQueue) {
      pthread_mutex_lock(&cl_data->mtx);
      _cl_drain(cl_data);
      pthread_mutex_unlock(&cl_data->mtx);
    }
  }
  pthread_mutex_unlock(&_ze_cls_mutex);
}

/* Drain the slot that most recently signaled `ev` (recursing on preds). */
static void _on_sync_drain_event(ze_event_handle_t ev) {
  struct _ze_slot *s = _latest_get(ev);
  if (!s || !s->owner)
    return;
  pthread_mutex_lock(&s->owner->mtx);
  _slot_drain(s);
  /* The drained slot may have left siblings live; only clear
   * in_flight_q if nothing in this cl remains in flight. */
  int any_live = 0;
  struct _ze_slab_chunk *c;
  DL_FOREACH(s->owner->chunks, c) {
    for (uint32_t i = 0; i < c->n_used; ++i)
      if (c->slots[i].live) {
        any_live = 1;
        break;
      }
    if (any_live)
      break;
  }
  if (!any_live)
    s->owner->in_flight_q = NULL;
  pthread_mutex_unlock(&s->owner->mtx);
}

/* Execute-epilogue handler for ONE cl. Runs AFTER L0's actual Execute
 * has returned, with the user cl in flight on its engine.
 *
 * Three phases, all under cl_data->mtx so a concurrent Execute or Sync
 * on another thread sees them atomically:
 *
 *   1) If in_flight_q is set from a prior Execute by *another* thread,
 *      force-sync that queue and drain the slab before we overwrite it
 *      (regression test: inorder_reg_Event_multithreaded_01 — same cl on two queues
 *      from two threads, expect both rounds' timings).
 *   2) (SHADOW PATH ONLY) Append a fresh Query on the per-(ctx,device)
 *      shadow cl for each slot. Must run AFTER L0 Execute (not before) —
 *      Appending on the shadow cl before the user cl is in flight
 *      deadlocks when the shadow shares the engine with the user cl
 *      (see tests/bugs/query_on_separate_cl_regular_user_cl). Inline
 *      (compute) cls have the QKT baked into the cl body at Append; it
 *      re-fires automatically on every Execute, no work here.
 *   3) Stamp in_flight_q = hQueue and instantiate each slot, publishing
 *      it to the dep graph + as the "owner" of this queue. */
static void _on_execute_one_cl(ze_command_queue_handle_t hQueue,
                               ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_ZE_CL(&command_list, cl_data);
  if (!cl_data)
    return;
  pthread_mutex_lock(&cl_data->mtx);

  if (cl_data->in_flight_q) {
    _ZE_MUST(ZE_COMMAND_QUEUE_SYNCHRONIZE_PTR(cl_data->in_flight_q, UINT64_MAX));
    _cl_drain(cl_data);
  }
  /* Shadow-path setup is skipped entirely for compute cls — those slots
   * have shadow_done==NULL and rely on the inline QKT in the cl body. */
  struct _ze_shadow_cl *sh = NULL;
  if (!cl_data->is_compute) {
    ze_context_handle_t ctx = _cl_cache_context(cl_data, command_list);
    ze_device_handle_t dev = _cl_cache_device(cl_data, command_list);
    sh = (ctx && dev) ? _get_shadow_cl(ctx, dev) : NULL;
  }
  struct _ze_slab_chunk *c;
  DL_FOREACH(cl_data->chunks, c) {
    for (uint32_t j = 0; j < c->n_used; ++j) {
      struct _ze_slot *slot = &c->slots[j];
      if (!slot->inj)
        continue;
      if (!cl_data->is_compute) {
        if (!sh || !slot->shadow_done)
          continue;
        _shadow_append_query(sh, slot->inj->event, c->slab, &slot->off, slot->shadow_done->event);
        slot->sh = sh;
      }
      _slot_instantiate(cl_data, slot);
    }
  }
  cl_data->in_flight_q = hQueue;

  pthread_mutex_unlock(&cl_data->mtx);
}

static void _on_execute_command_lists_epilogue(ze_command_queue_handle_t hQueue,
                                               uint32_t numCommandLists,
                                               ze_command_list_handle_t *phCommandLists) {
  for (uint32_t i = 0; i < numCommandLists; ++i)
    _on_execute_one_cl(hQueue, phCommandLists[i]);
}

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

////////////////////////////////////////////
#define _ZE_ERROR_MSG(NAME, RES)                                                                   \
  do {                                                                                             \
    fprintf(stderr, "%s() failed at %d(%s): res=%x\n", (NAME), __LINE__, __FILE__, (RES));         \
  } while (0)
#define _ZE_ERROR_MSG_NOTERMINATE(NAME, RES)                                                       \
  do {                                                                                             \
    fprintf(stderr, "%s() error at %d(%s): res=%x\n", (NAME), __LINE__, __FILE__, (RES));          \
  } while (0)
#define _ERROR_MSG(MSG)                                                                            \
  {                                                                                                \
    perror((MSG)) do {                                                                             \
      {                                                                                            \
        perror((MSG));                                                                             \
        fprintf(stderr, "errno=%d at %d(%s)", errno, __LINE__, __FILE__);                          \
      }                                                                                            \
      while (0)

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
