/* Algorithm
 * =========
 *
 * On profiled Append (cl, sig=user_sig, waits=user_waits):
 *   - allocate inj from per-context pool; swap user_sig -> inj
 *   - insert Query(wait=inj, sig=user_sig, slab[off])
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
 *   - read slab[s.off], emit tracepoint(s.attr or inj)
 *   - clear latest[s.attr] (if it still points at s)
 *   - clear s.live and s.preds
 *   (Build-time fields inj, attr, off, waits stay so the next Execute
 *    can re-instantiate without re-Appending.)
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

/* Dependency-tracking slot: one per profiled Append. Slots carry the
 * happens-before edges the user established (via cl in-order semantics
 * and via phWaitEvents). At sync time we walk these edges from the
 * synced anchor and drain everything reachable. Drain is pop semantics:
 * after emit, the slot is dropped from the cl's list. */
struct _ze_slot {
  struct _ze_command_list_obj_data *owner; /* cl_data this slot lives in (==> .slab to read at drain) */
  struct _ze_event_h *inj;             /* tracer-owned event the Query waits on */
  struct _ze_event_h *shadow_done;     /* tracer-owned event the Query signals; drain host-syncs on this */
  ze_event_handle_t   attr;            /* user's original signal event (NULL => inj->event) */
  size_t              off;             /* byte offset within owner->slab */
  /* User wait events copied at Append time (stable across rebuilds);
   * preds[] is computed at instantiate from waits[] by looking up
   * latest[w] for each w. */
  ze_event_handle_t  *waits;
  uint32_t            n_waits;
  struct _ze_slot   **preds;           /* points at slots whose drain must come first (may be in another cl) */
  uint32_t            n_preds;
  unsigned char       live;            /* in-flight (instantiated, not drained) */
};

#define _ZE_SLAB_SLOTS_INITIAL 64

struct _ze_command_list_obj_data {
  void *ptr;
  UT_hash_handle hh;

  void              *slab;       /* host-visible KT result buffer; alloc'd once, leaked on destroy */
  struct _ze_slot   *slots;
  uint32_t           n_slots;

  /* in_flight_q is the queue this cl was last Executed on AND not yet
   * drained. NULL means "not in flight" — safe to Execute without a
   * force-sync. Set on Execute, cleared on drain.
   *
   * Held only for regular cls; immediate cls never Execute. */
  ze_command_queue_handle_t in_flight_q;
  /* Serializes the Execute prologue: if two threads race to Execute the
   * same closed cl on different queues, we need to force-sync the prior
   * one before letting the second run instantiate. */
  pthread_mutex_t    mtx;
  unsigned char      is_immediate;
  unsigned char      is_in_order;

  /* Cached on first use: device handle and context handle for this cl.
   * Both are immutable for the life of the cl, so caching avoids the
   * per-Append/per-Execute ZE_*_GET_*_HANDLE_PTR roundtrips. */
  ze_device_handle_t  cached_device;
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

/* Per-device cache of the first COMPUTE queue group ordinal. The lookup
 * is read-mostly: scan zeDeviceGetCommandQueueGroupProperties once,
 * remember the answer. valid=0 means "we already checked and there's no
 * compute group on this device" — treated as fatal at use sites. */
struct _ze_compute_ord_entry {
  ze_device_handle_t device;
  uint32_t           ordinal;
  unsigned char      valid;
  UT_hash_handle     hh;
};
static struct _ze_compute_ord_entry *_ze_compute_ords = NULL;
static pthread_mutex_t _ze_compute_ords_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Returns the first COMPUTE queue group ordinal for device, or (uint32_t)-1
 * if the device exposes no compute group (fatal — caller should bail). */
static uint32_t _get_compute_ordinal(ze_device_handle_t device) {
  pthread_mutex_lock(&_ze_compute_ords_mutex);
  struct _ze_compute_ord_entry *e = NULL;
  HASH_FIND_PTR(_ze_compute_ords, &device, e);
  if (e) {
    uint32_t r = e->valid ? e->ordinal : (uint32_t)-1;
    pthread_mutex_unlock(&_ze_compute_ords_mutex);
    return r;
  }
  pthread_mutex_unlock(&_ze_compute_ords_mutex);

  /* Slow path: scan queue groups outside the lock. */
  uint32_t n_groups = 0;
  if (ZE_DEVICE_GET_COMMAND_QUEUE_GROUP_PROPERTIES_PTR(device, &n_groups, NULL)
      != ZE_RESULT_SUCCESS || n_groups == 0)
    return (uint32_t)-1;
  ze_command_queue_group_properties_t *groups =
      (ze_command_queue_group_properties_t *)calloc(n_groups, sizeof(*groups));
  if (!groups) return (uint32_t)-1;
  for (uint32_t i = 0; i < n_groups; ++i)
    groups[i].stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
  uint32_t found = (uint32_t)-1;
  if (ZE_DEVICE_GET_COMMAND_QUEUE_GROUP_PROPERTIES_PTR(device, &n_groups, groups)
      == ZE_RESULT_SUCCESS) {
    for (uint32_t i = 0; i < n_groups; ++i)
      if (groups[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        found = i; break;
      }
  }
  free(groups);

  pthread_mutex_lock(&_ze_compute_ords_mutex);
  /* Re-check under the lock — another thread may have populated. */
  HASH_FIND_PTR(_ze_compute_ords, &device, e);
  if (!e) {
    e = (struct _ze_compute_ord_entry *)calloc(1, sizeof(*e));
    if (e) {
      e->device  = device;
      e->ordinal = found;
      e->valid   = (found != (uint32_t)-1) ? 1 : 0;
      HASH_ADD_PTR(_ze_compute_ords, device, e);
    }
  } else {
    found = e->valid ? e->ordinal : (uint32_t)-1;
  }
  pthread_mutex_unlock(&_ze_compute_ords_mutex);
  return found;
}

/* Per-(context, device) tracer-owned immediate OOO compute cl used to
 * host the AppendQueryKernelTimestamps op. The Query can't live on the
 * user's cl when that cl is on a copy-only queue group (driver aborts),
 * and we use the shadow cl uniformly for all engines so the code path
 * is identical regardless of user-cl kind. */
struct _ze_shadow_key {
  ze_context_handle_t context;
  ze_device_handle_t  device;
};
struct _ze_shadow_cl {
  struct _ze_shadow_key    key;
  ze_command_list_handle_t cl;
  pthread_mutex_t          mtx;
  UT_hash_handle           hh;
};
static struct _ze_shadow_cl *_ze_shadow_cls = NULL;
static pthread_mutex_t _ze_shadow_cls_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Returns the shadow cl for (context, device), creating it lazily on
 * first use. Returns NULL if the device has no compute group (fatal:
 * we log to stderr) or if creation fails. */
static struct _ze_shadow_cl *_get_shadow_cl(ze_context_handle_t context,
                                            ze_device_handle_t device) {
  struct _ze_shadow_key key = { context, device };
  pthread_mutex_lock(&_ze_shadow_cls_mutex);
  struct _ze_shadow_cl *sh = NULL;
  HASH_FIND(hh, _ze_shadow_cls, &key, sizeof(key), sh);
  if (sh) { pthread_mutex_unlock(&_ze_shadow_cls_mutex); return sh; }
  pthread_mutex_unlock(&_ze_shadow_cls_mutex);

  /* Slow path: create outside the registry lock. */
  uint32_t ord = _get_compute_ordinal(device);
  if (ord == (uint32_t)-1) {
    fprintf(stderr, "THAPI: device %p has no COMPUTE queue group; "
                    "cannot create shadow cl. Profiling disabled for "
                    "command lists on this device.\n", (void *)device);
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
      ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC, NULL, ord, 0, 0,
      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL };
  ze_command_list_handle_t new_cl = NULL;
  if (ZE_COMMAND_LIST_CREATE_IMMEDIATE_PTR(context, device, &qd, &new_cl)
        != ZE_RESULT_SUCCESS || !new_cl) {
    fprintf(stderr, "THAPI: failed to create shadow cl for "
                    "context=%p device=%p\n", (void *)context, (void *)device);
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
  sh->cl  = new_cl;
  pthread_mutex_init(&sh->mtx, NULL);
  HASH_ADD(hh, _ze_shadow_cls, key, sizeof(sh->key), sh);
  pthread_mutex_unlock(&_ze_shadow_cls_mutex);
  return sh;
}

/* Append AppendQueryKernelTimestamps on the shadow cl: wait on inj,
 * signal shadow_done, write timestamps into slab[*off]. Serialized on
 * sh->mtx because L0 doesn't allow concurrent Appends to one cl.
 * Returns 0 on success, -1 on failure. */
static int _shadow_append_query(struct _ze_shadow_cl *sh,
                                ze_event_handle_t inj_event,
                                void *slab, size_t *off,
                                ze_event_handle_t shadow_done_event) {
  pthread_mutex_lock(&sh->mtx);
  ze_result_t r = ZE_COMMAND_LIST_APPEND_QUERY_KERNEL_TIMESTAMPS_PTR(
      sh->cl, 1, &inj_event, slab, off,
      /*hSignalEvent=*/ shadow_done_event,
      /*numWaitEvents=*/ 1, &inj_event);
  pthread_mutex_unlock(&sh->mtx);
  return (r == ZE_RESULT_SUCCESS) ? 0 : -1;
}

static inline void _on_create_command_list(ze_command_list_handle_t command_list,
                                            int immediate, int in_order) {
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
  ze_event_handle_t ev;     /* key */
  struct _ze_slot  *slot;
  UT_hash_handle    hh;
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
  if (!ev) return;
  pthread_mutex_lock(&_ze_latest_mutex);
  struct _ze_latest_entry *e = NULL;
  HASH_FIND_PTR(_ze_latest, &ev, e);
  if (!e) {
    e = (struct _ze_latest_entry *)calloc(1, sizeof(*e));
    if (!e) { pthread_mutex_unlock(&_ze_latest_mutex); return; }
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
  if (!ev) return;
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
        THAPI_DBGLOG("Failed to allocate memory");                                         \
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
    ZE_EVENT_HOST_RESET_PTR(val->event);                                                           \
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

/* Allocate one new slot at the end of the cl's slot list. Slots are
 * never reused within a cl's lifetime — the cl body's Query op
 * hard-codes inj and off; the slot is the host-side mirror that gets
 * re-instantiated on every Execute.
 *
 * Capacity is fixed at _ZE_SLAB_SLOTS_INITIAL to keep slot addresses
 * stable for the cl's lifetime. We store raw slot pointers in
 * `latest[ev] -> slot` and in other slots' `preds[]`; realloc would
 * invalidate every one of them, silently breaking dep-graph walks
 * (see tests/bugs/missing_drain_dag). The slab is sized to match, so
 * growing slots beyond it would gain nothing anyway.
 *
 * Allocations (slots array and slab) happen BEFORE n_slots is bumped,
 * so an OOM does not leave a hole in the slot indexing. */
static struct _ze_slot *_cl_slot_append(struct _ze_command_list_obj_data *cl_data,
                                        ze_context_handle_t ctx,
                                        struct _ze_event_h *inj,
                                        struct _ze_event_h *shadow_done,
                                        ze_event_handle_t attr,
                                        ze_event_handle_t *waits,
                                        uint32_t n_waits) {
  if (cl_data->n_slots >= _ZE_SLAB_SLOTS_INITIAL) return NULL;
  if (!cl_data->slots) {
    cl_data->slots = (struct _ze_slot *)calloc(
        _ZE_SLAB_SLOTS_INITIAL, sizeof(struct _ze_slot));
    if (!cl_data->slots) return NULL;
  }
  if (!cl_data->slab) {
    size_t bytes = (size_t)_ZE_SLAB_SLOTS_INITIAL * sizeof(ze_kernel_timestamp_result_t);
    ze_host_mem_alloc_desc_t hd = {ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, NULL, 0};
    void *buf = NULL;
    if (ZE_MEM_ALLOC_HOST_PTR(ctx, &hd, bytes, sizeof(uint64_t), &buf) != ZE_RESULT_SUCCESS || !buf)
      return NULL;
    memset(buf, 0, bytes);
    cl_data->slab = buf;
  }
  uint32_t idx = cl_data->n_slots;
  struct _ze_slot *s = &cl_data->slots[idx];
  s->owner       = cl_data;
  s->inj         = inj;
  s->shadow_done = shadow_done;
  s->attr        = attr;
  s->off   = (size_t)idx * sizeof(ze_kernel_timestamp_result_t);
  s->live  = 0;
  s->preds = NULL; s->n_preds = 0;
  if (n_waits) {
    s->waits = (ze_event_handle_t *)malloc(n_waits * sizeof(ze_event_handle_t));
    if (s->waits) {
      memcpy(s->waits, waits, n_waits * sizeof(ze_event_handle_t));
      s->n_waits = n_waits;
    } else { s->n_waits = 0; }
  } else { s->waits = NULL; s->n_waits = 0; }
  cl_data->n_slots++;
  return s;
}

/* Compute s->preds from s->waits via the global latest[] map, plus the
 * previous live slot on this cl if the cl is in-order. Marks s live and
 * publishes s as the new latest[attr]. */
static void _slot_instantiate(struct _ze_command_list_obj_data *cl_data,
                              struct _ze_slot *s) {
  s->live = 1;
  uint32_t cap = s->n_waits + 1; /* +1 for in-order prev */
  s->preds = (struct _ze_slot **)calloc(cap, sizeof(struct _ze_slot *));
  s->n_preds = 0;
  for (uint32_t i = 0; i < s->n_waits; ++i) {
    struct _ze_slot *p = _latest_get(s->waits[i]);
    if (p && p->live) s->preds[s->n_preds++] = p;
  }
  if (cl_data->is_in_order) {
    /* Find previous live slot in this cl (by slot index lower than s). */
    uint32_t self = (uint32_t)(s - cl_data->slots);
    for (int32_t i = (int32_t)self - 1; i >= 0; --i) {
      if (cl_data->slots[i].live) {
        s->preds[s->n_preds++] = &cl_data->slots[i];
        break;
      }
    }
  }
  if (s->attr) _latest_set(s->attr, s);
}

/* Append-time hook called from profiling_epilogue. Caller already
 * swapped user's hSignalEvent for inj->event. user_signal is the
 * ORIGINAL value (possibly NULL). user_waits is the user's wait list
 * (NULL,0 if none).
 *
 * Inserts a Query waiting on inj, signaling user_signal. For immediate
 * cls, instantiates the slot inline (immediate Appends fire as soon as
 * appended). For regular cls, the slot is created but not instantiated
 * until Execute. */
static void _universal_record_append(ze_command_list_handle_t command_list,
                                     struct _ze_event_h *inj,
                                     ze_event_handle_t user_signal,
                                     ze_event_handle_t *user_waits,
                                     uint32_t user_n_waits) {
  if (!inj) return;
  struct _ze_event_h *shadow_done = NULL;
  struct _ze_command_list_obj_data *cl_data = NULL;
  struct _ze_slot *s = NULL;

  ze_context_handle_t ctx = NULL;
  if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &ctx) != ZE_RESULT_SUCCESS || !ctx)
    goto fail;
  inj->context = ctx;

  /* Tracer-owned fence event: Query signals it, drain host-waits on it
   * before reading the slab. Decouples drain-time correctness from any
   * user sync on user_signal — required because in step 2 the Query
   * moves to a separate shadow cl whose completion isn't implied by
   * user-level sync. */
  shadow_done = _get_profiling_event(command_list);
  if (!shadow_done) goto fail;
  shadow_done->context = ctx;

  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) goto fail;
  pthread_mutex_lock(&cl_data->mtx);

  s = _cl_slot_append(cl_data, ctx, inj, shadow_done,
                      user_signal, user_waits, user_n_waits);
  if (!s) goto fail_locked;

  ze_event_handle_t wait_ev = inj->event;

  /* Chain user_signal off inj so the user's wait still completes once
   * the underlying op has fired. We swapped user's hSignalEvent for
   * inj in the prologue, so nothing else on this cl signals
   * user_signal. AppendBarrier (not AppendSignalEvent) because we need
   * to both wait on inj and signal user_signal; SignalEvent doesn't
   * take a wait list. Skipped when user passed NULL. */
  if (user_signal) {
    if (ZE_COMMAND_LIST_APPEND_BARRIER_PTR(command_list, user_signal, 1, &wait_ev)
        != ZE_RESULT_SUCCESS)
      goto fail_locked;
  }

  /* The Query Append now lives on the per-(context, device) shadow
   * compute cl rather than the user cl. This is what lets us profile
   * copy-only user cls — copy engines reject AppendQueryKernelTimestamps
   * but the shadow cl is always compute. For regular user cls we defer
   * the Append to Execute prologue (the user cl hasn't run yet, so
   * nothing is signaling inj — Appending the Query on an immediate
   * shadow cl now would let it fire too early on a stale inj). */
  if (cl_data->is_immediate) {
    cl_data->cached_context = ctx;
    ze_device_handle_t dev = cl_data->cached_device;
    if (!dev &&
        ZE_COMMAND_LIST_GET_DEVICE_HANDLE_PTR(command_list, &dev) == ZE_RESULT_SUCCESS)
      cl_data->cached_device = dev;
    struct _ze_shadow_cl *sh = dev ? _get_shadow_cl(ctx, dev) : NULL;
    if (!sh || _shadow_append_query(sh, inj->event, cl_data->slab, &s->off,
                                     shadow_done->event) != 0)
      goto fail_locked;
    _slot_instantiate(cl_data, s);
  }
  pthread_mutex_unlock(&cl_data->mtx);
  ADD_ZE_CL(cl_data);
  return;

fail_locked:
  if (s) { free(s->waits); cl_data->n_slots--; }
  pthread_mutex_unlock(&cl_data->mtx);
  ADD_ZE_CL(cl_data);
fail:
  if (shadow_done) PUT_ZE_EVENT(shadow_done);
  PUT_ZE_EVENT(inj);
}

/* Drain one slot. Recurses on its preds first, then emits this slot
 * and pops it. Pop = clear inj/waits/preds; the holed entry is reused
 * by later _cl_slot_append calls. Safe to call on already-drained
 * (live=0) slot.
 *
 * Reads use s->owner->slab — preds may live in a different cl than the
 * caller (cross-cl signal chains), so we cannot use the caller's slab.
 *
 * Locking: a pred on another cl is read/mutated WITHOUT taking its
 * owner's mtx. That's safe in the current model because slot pointers
 * are stable (cap is fixed, never realloc'd) and live-flag clearing
 * races are benign — the worst case is one extra tracepoint emit, not
 * a UAF. Take the pred's mtx only if we ever start freeing slot arrays.
 *
 * No cycle guard: cycles are impossible by construction. preds come
 * from two sources:
 *   - in-order prev slot in the same cl: strictly lower slot index, DAG.
 *   - latest[wait_event]: a slot published BEFORE us. Forming a cycle
 *     requires the user to declare two Appends each waiting on the
 *     other's signal event — L0 itself would deadlock the GPU on that,
 *     so we would never observe a sync return to reach drain. */
static void _slot_drain(struct _ze_slot *s) {
  if (!s || !s->live) return;
  for (uint32_t i = 0; i < s->n_preds; ++i)
    _slot_drain(s->preds[i]);
  s->live = 0;
  /* Block until our Query op has actually fired, then reset the fence
   * so the next Execute round starts with a clean event. We can't
   * trust the caller's sync to have covered the Query — in step 2 the
   * Query will live on a separate shadow cl, and even in step 1 this
   * makes the slab read unconditional rather than relying on cl-order
   * implications. */
  if (s->shadow_done && s->shadow_done->event) {
    ZE_EVENT_HOST_SYNCHRONIZE_PTR(s->shadow_done->event, UINT64_MAX);
    ZE_EVENT_HOST_RESET_PTR(s->shadow_done->event);
  }
  ze_event_handle_t attr = s->attr ? s->attr : (s->inj ? s->inj->event : NULL);
  if (s->owner && s->owner->slab && attr &&
      tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results)) {
    ze_kernel_timestamp_result_t r =
        *(ze_kernel_timestamp_result_t *)((char *)s->owner->slab + s->off);
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, attr,
                  ZE_RESULT_SUCCESS, ZE_RESULT_SUCCESS,
                  r.global.kernelStart, r.global.kernelEnd,
                  r.context.kernelStart, r.context.kernelEnd);
  }
  _latest_clear_if(s->attr, s);
  /* Per-run preds reset; build-time fields (inj, attr, off, waits) stay
   * so the next Execute can re-instantiate without re-Append. */
  free(s->preds); s->preds = NULL; s->n_preds = 0;
}

/* Drain every live slot in a cl. */
static void _cl_drain(struct _ze_command_list_obj_data *cl_data) {
  for (uint32_t i = 0; i < cl_data->n_slots; ++i)
    _slot_drain(&cl_data->slots[i]);
  cl_data->in_flight_q = NULL;
}

/* Drain a single cl. */
static void _on_sync_drain_cl(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_ZE_CL(&command_list, cl_data);
  if (!cl_data) return;
  pthread_mutex_lock(&cl_data->mtx);
  _cl_drain(cl_data);
  pthread_mutex_unlock(&cl_data->mtx);
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
  if (!s || !s->owner) return;
  pthread_mutex_lock(&s->owner->mtx);
  _slot_drain(s);
  /* The drained slot may have left siblings live; only clear
   * in_flight_q if nothing in this cl remains in flight. */
  int any_live = 0;
  for (uint32_t i = 0; i < s->owner->n_slots; ++i)
    if (s->owner->slots[i].live) { any_live = 1; break; }
  if (!any_live) s->owner->in_flight_q = NULL;
  pthread_mutex_unlock(&s->owner->mtx);
}

/* zeCommandQueueExecuteCommandLists EPILOGUE — runs AFTER L0's actual
 * Execute has returned, with the user cl in flight on its engine.
 *
 * Three things happen here, all under cl_data->mtx so a concurrent
 * Execute (or Sync) on another thread sees them atomically:
 *
 *   1) If in_flight_q is set from a prior Execute by *another* thread,
 *      force-sync that queue and drain the slab before we overwrite it
 *      (regression test: inorder_reg_Event_11 — same cl on two queues
 *      from two threads, expect both rounds' timings).
 *   2) Append a fresh Query on the per-(ctx,device) shadow cl for each
 *      slot. Must run AFTER L0 Execute (not before) — Appending on the
 *      shadow cl before the user cl is in flight deadlocks when the
 *      shadow shares the engine with the user cl (see
 *      tests/bugs/query_on_separate_cl_regular_user_cl).
 *   3) Stamp in_flight_q = hQueue and instantiate the slot, publishing
 *      it to the dep graph + as the "owner" of this queue. */
static void _on_execute_command_lists_epilogue(ze_command_queue_handle_t hQueue,
                                                uint32_t numCommandLists,
                                                ze_command_list_handle_t *phCommandLists) {
  for (uint32_t i = 0; i < numCommandLists; ++i) {
    struct _ze_command_list_obj_data *cl_data = NULL;
    FIND_ZE_CL(phCommandLists + i, cl_data);
    if (!cl_data) continue;
    pthread_mutex_lock(&cl_data->mtx);
    if (cl_data->in_flight_q) {
      ZE_COMMAND_QUEUE_SYNCHRONIZE_PTR(cl_data->in_flight_q, UINT64_MAX);
      _cl_drain(cl_data);
    }
    ze_context_handle_t ctx = cl_data->cached_context;
    if (!ctx) {
      if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(phCommandLists[i], &ctx) == ZE_RESULT_SUCCESS)
        cl_data->cached_context = ctx;
    }
    ze_device_handle_t dev = cl_data->cached_device;
    if (!dev &&
        ZE_COMMAND_LIST_GET_DEVICE_HANDLE_PTR(phCommandLists[i], &dev) == ZE_RESULT_SUCCESS)
      cl_data->cached_device = dev;
    struct _ze_shadow_cl *sh = (ctx && dev) ? _get_shadow_cl(ctx, dev) : NULL;
    for (uint32_t j = 0; j < cl_data->n_slots; ++j) {
      struct _ze_slot *slot = &cl_data->slots[j];
      if (!sh || !slot->inj || !slot->shadow_done) continue;
      if (_shadow_append_query(sh, slot->inj->event, cl_data->slab,
                               &slot->off, slot->shadow_done->event) != 0)
        continue;  /* slot stays not-live this round; we miss this timing */
      _slot_instantiate(cl_data, slot);
    }
    cl_data->in_flight_q = hQueue;
    pthread_mutex_unlock(&cl_data->mtx);
  }
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
