#ifdef THAPI_DEBUG
#define TAHPI_LOG stderr
#define THAPI_DBGLOG(fmt, ...)                                                                     \
  do {                                                                                             \
    fprintf(TAHPI_LOG, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__, __VA_ARGS__);                \
  } while (0)
#define THAPI_DBGLOG_NO_ARGS(fmt)                                                                  \
  do {                                                                                             \
    fprintf(TAHPI_LOG, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__);                             \
  } while (0)
#else
#define THAPI_DBGLOG(...)                                                                          \
  do {                                                                                             \
  } while (0)
#define THAPI_DBGLOG_NO_ARGS(fmt)                                                                  \
  do {                                                                                             \
  } while (0)
#endif

#ifdef THAPI_USE_DESTRUCTORS
#define THAPI_ATTRIBUTE_DESTRUCTOR __attribute__((destructor))
#else
#define THAPI_ATTRIBUTE_DESTRUCTOR
#endif

static int _do_profile = 0;
static int _do_cleanup = 0;
static int _do_chained_structs = 0;
static int _do_paranoid_drift = 0;
static int _do_paranoid_memory_location = 0;
static int _do_ddi_table_forward = 0;
/* When THAPI_REPORT_INJECTED_EVENTS=1, _lib_cleanup prints the running
 * total to stderr. Useful for the bats infra to assert we don't inject
 * more events than necessary (lazy fallback regression guard). */
static int _do_report_injected_events = 0;
static volatile uint64_t _injected_event_count = 0;

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

/* Universal per-Append scheme bookkeeping (see project_ze_universal_scheme):
 * one slot per profiled Append in the cl. Each slot holds the injected
 * event we swapped in (wrapper), the original user signal event we'll
 * attribute the timestamp to at drain time (NULL if user passed no
 * event — attribute to inj instead), and the offset within the cl's
 * slab buffer where the Query writes the timestamp. */
struct _ze_slot {
  struct _ze_event_h *inj;             /* tracer-owned event swapped into the Append */
  ze_event_handle_t   attr;            /* event to attribute the timestamp to at drain (NULL => inj->event) */
  size_t              off;             /* byte offset within cl_data->slab */
};

#define _ZE_SLAB_SLOTS_INITIAL 64

struct _ze_command_list_obj_data {
  void *ptr; /* the ze_command_list_handle_t this entry tracks */
  UT_hash_handle hh;

  /* Universal scheme state — populated lazily on first profiled Append. */
  void              *slab;       /* host-visible buffer for Query writes */
  size_t             slab_bytes; /* allocated size in bytes */
  ze_context_handle_t slab_ctx;  /* context the slab is allocated on (for free) */
  struct _ze_slot   *slots;
  uint32_t           n_slots;
  uint32_t           cap_slots;
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

static inline void _on_create_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;

  FIND_ZE_CL(&command_list, cl_data);
  if (cl_data) {
    THAPI_DBGLOG("Command list already registered: %p", command_list);
    return;
  }

  cl_data = (struct _ze_command_list_obj_data *)calloc(1, sizeof(*cl_data));
  if (!cl_data) {
    THAPI_DBGLOG_NO_ARGS("Failed to allocate memory");
    return;
  }
  cl_data->ptr = (void *)command_list;
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
        THAPI_DBGLOG_NO_ARGS("Failed to allocate memory");                                         \
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
  if (_do_report_injected_events)
    __sync_fetch_and_add(&_injected_event_count, 1);
  return e_w;
cleanup_ep:
  ZE_EVENT_POOL_DESTROY_PTR(e_w->event_pool);
cleanup_wrapper:
  PUT_ZE_EVENT_WRAPPER(e_w);
  return NULL;
}

/* Emit an event_profiling_results tracepoint directly from a captured
 * ze_kernel_timestamp_result_t (no driver Query). Used by the universal
 * scheme's drain path: the Query already wrote the timestamp into the
 * slab buffer, so we just read the slot and emit. */
static inline void _emit_kts_tracepoint(ze_event_handle_t attr_event,
                                        const ze_kernel_timestamp_result_t *r) {
  if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results))
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, attr_event,
                  ZE_RESULT_SUCCESS, ZE_RESULT_SUCCESS,
                  r->global.kernelStart, r->global.kernelEnd,
                  r->context.kernelStart, r->context.kernelEnd);
}

/* Universal scheme: ensure the cl's slab buffer is large enough to hold
 * `n_slots` timestamps. First call allocates a host-visible buffer in
 * `ctx`; later calls grow if needed. Returns 0 on success. */
static int _cl_slab_ensure(struct _ze_command_list_obj_data *cl_data,
                           ze_context_handle_t ctx, uint32_t n_slots) {
  size_t needed = (size_t)n_slots * sizeof(ze_kernel_timestamp_result_t);
  if (cl_data->slab && cl_data->slab_bytes >= needed)
    return 0;
  if (cl_data->slab) {
    /* Outgrew the initial slab. For now we only allocate the initial size
     * (capacity is bumped via realloc of the slot array; the slab itself
     * is sized once). If we hit this path, it means more profiled Appends
     * than _ZE_SLAB_SLOTS_INITIAL in a single cl — bail rather than
     * realloc a host-visible alloc (no safe way to do that mid-record). */
    THAPI_DBGLOG("slab full for cl %p (have %zu bytes, need %zu)",
                 cl_data->ptr, cl_data->slab_bytes, needed);
    return -1;
  }
  size_t bytes = (size_t)_ZE_SLAB_SLOTS_INITIAL * sizeof(ze_kernel_timestamp_result_t);
  ze_host_mem_alloc_desc_t hd = {ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, NULL, 0};
  void *buf = NULL;
  if (ZE_MEM_ALLOC_HOST_PTR(ctx, &hd, bytes, sizeof(uint64_t), &buf) != ZE_RESULT_SUCCESS ||
      !buf) {
    THAPI_DBGLOG("zeMemAllocHost(slab) failed for cl %p", cl_data->ptr);
    return -1;
  }
  memset(buf, 0, bytes);
  cl_data->slab = buf;
  cl_data->slab_bytes = bytes;
  cl_data->slab_ctx = ctx;
  return 0;
}

/* Universal scheme: grow the slot array if full. */
static inline int _cl_slots_grow(struct _ze_command_list_obj_data *cl_data) {
  if (cl_data->n_slots < cl_data->cap_slots) return 0;
  uint32_t new_cap = cl_data->cap_slots ? cl_data->cap_slots * 2 : 8;
  struct _ze_slot *grown = (struct _ze_slot *)realloc(
      cl_data->slots, new_cap * sizeof(struct _ze_slot));
  if (!grown) return -1;
  cl_data->slots = grown;
  cl_data->cap_slots = new_cap;
  return 0;
}

/* Universal scheme: record one new slot on this cl. Caller will issue
 * the actual zeCommandListAppendQueryKernelTimestamps with the returned
 * offset. Returns NULL on failure (caller should not insert the Query). */
static struct _ze_slot *_cl_slot_append(struct _ze_command_list_obj_data *cl_data,
                                        ze_context_handle_t ctx,
                                        struct _ze_event_h *inj,
                                        ze_event_handle_t attr) {
  if (_cl_slots_grow(cl_data) != 0) return NULL;
  if (_cl_slab_ensure(cl_data, ctx, cl_data->n_slots + 1) != 0) return NULL;
  struct _ze_slot *s = &cl_data->slots[cl_data->n_slots++];
  s->inj  = inj;
  s->attr = attr;
  s->off  = (size_t)(cl_data->n_slots - 1) * sizeof(ze_kernel_timestamp_result_t);
  return s;
}

/* Universal scheme — append-time hook called from profiling_epilogue.
 *
 * Postconditions on success:
 *   - One zeCommandListAppendQueryKernelTimestamps appended to the cl,
 *     waiting on `inj`'s event and signaling `user_signal` (NULL = no
 *     signal). Its dst byte-offset within cl_data->slab is recorded in
 *     a new slot.
 *   - The slot's `attr` is set to user_signal (or NULL → attribute to
 *     inj at drain time), so iprof gets one event_profiling_results per
 *     profiled Append.
 *
 * On failure (no cl_data, no context, slab/slot alloc failed, Query
 * failed): the injected wrapper is released back to the pool and no
 * Query is added. The user's Append already happened; we just lose the
 * timestamp for this one.
 *
 * Caller has already swapped the user's hSignalEvent for inj->event.
 * `user_signal` is the ORIGINAL value (possibly NULL). */
static void _universal_record_append(ze_command_list_handle_t command_list,
                                     struct _ze_event_h *inj,
                                     ze_event_handle_t user_signal) {
  if (!inj) return;

  ze_context_handle_t ctx = NULL;
  if (ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &ctx) != ZE_RESULT_SUCCESS || !ctx) {
    PUT_ZE_EVENT(inj);
    return;
  }
  /* Stamp the wrapper's context so PUT_ZE_EVENT can route it back to the
   * correct per-context pool at drain. */
  inj->context = ctx;

  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    PUT_ZE_EVENT(inj);
    return;
  }

  struct _ze_slot *slot = _cl_slot_append(cl_data, ctx, inj, user_signal);
  if (!slot) {
    ADD_ZE_CL(cl_data);
    PUT_ZE_EVENT(inj);
    return;
  }

  /* Insert the Query into the cmdlist body. wait=inj so the Query runs
   * after the user's op (which signals inj); signal=user_signal so user
   * code that waits on user_signal still sees a signal. */
  ze_event_handle_t wait_ev = inj->event;
  ze_result_t r = ZE_COMMAND_LIST_APPEND_QUERY_KERNEL_TIMESTAMPS_PTR(
      command_list, 1, &wait_ev, cl_data->slab, &slot->off,
      /*hSignalEvent=*/ user_signal,
      /*numWaitEvents=*/ 1, &wait_ev);
  if (r != ZE_RESULT_SUCCESS) {
    /* Roll the slot back so drain doesn't read garbage. */
    cl_data->n_slots--;
    ADD_ZE_CL(cl_data);
    PUT_ZE_EVENT(inj);
    return;
  }
  ADD_ZE_CL(cl_data);
}

/* Universal scheme: drain captured timestamps from cl's slab and emit a
 * tracepoint per slot. Resets slot count but keeps the slab + capacity
 * for reuse on the next build. Called from sync hooks (post-Execute /
 * post-Sync). Safe to call when nothing's pending — returns immediately. */
static void _cl_drain(struct _ze_command_list_obj_data *cl_data) {
  if (cl_data->n_slots == 0) return;
  if (!cl_data->slab) {
    cl_data->n_slots = 0;
    return;
  }
  for (uint32_t i = 0; i < cl_data->n_slots; ++i) {
    struct _ze_slot *s = &cl_data->slots[i];
    ze_kernel_timestamp_result_t r =
        *(ze_kernel_timestamp_result_t *)((char *)cl_data->slab + s->off);
    ze_event_handle_t attr = s->attr ? s->attr : (s->inj ? s->inj->event : NULL);
    if (attr)
      _emit_kts_tracepoint(attr, &r);
    /* Release the injected wrapper back to the per-context pool. The
     * wrapper's event/pool stay alive in the pool so the next Append on
     * any cl in this context can recycle them. */
    if (s->inj)
      PUT_ZE_EVENT(s->inj);
  }
  cl_data->n_slots = 0;
}

/* Tear down a wrapper: destroy our injected event+pool if we own them,
 * then recycle the wrapper. Caller has already removed it from the
 * per-context free pool. */
static inline void _dispose_event_wrapper(struct _ze_event_h *ze_event) {
  if (ze_event->event_pool) {
    if (ze_event->event)
      ZE_EVENT_DESTROY_PTR(ze_event->event);
    ZE_EVENT_POOL_DESTROY_PTR(ze_event->event_pool);
  }
  PUT_ZE_EVENT_WRAPPER(ze_event);
}

static void _on_destroy_context(ze_context_handle_t context) {
  /* Free the per-context event-wrapper pool. All wrappers in it are idle
   * (returned via PUT_ZE_EVENT), so just dispose them. */
  pthread_mutex_lock(&_ze_event_pools_mutex);
  struct _ze_event_pool_entry *pool = NULL;
  HASH_FIND_PTR(_ze_event_pools, &context, pool);
  if (pool) {
    HASH_DEL(_ze_event_pools, pool);
    struct _ze_event_h *elt = NULL, *tmp = NULL;
    DL_FOREACH_SAFE(pool->events, elt, tmp) {
      DL_DELETE(pool->events, elt);
      _dispose_event_wrapper(elt);
    }
    free(pool);
  }
  pthread_mutex_unlock(&_ze_event_pools_mutex);
}

/* Universal scheme: free the cl's slab buffer (if allocated). Caller has
 * already drained the slots. Idempotent. */
static void _cl_slab_free(struct _ze_command_list_obj_data *cl_data) {
  if (cl_data->slab) {
    if (ZE_MEM_FREE_PTR && cl_data->slab_ctx)
      ZE_MEM_FREE_PTR(cl_data->slab_ctx, cl_data->slab);
    cl_data->slab = NULL;
    cl_data->slab_bytes = 0;
    cl_data->slab_ctx = NULL;
  }
}

/* Universal scheme: drain a cl by handle, used by sync hooks. Walks the
 * cl hash to find the cl_data, then drains it. Safe if cl_data is gone
 * (e.g. raced with destroy) — just no-ops. */
static void _on_sync_drain_cl(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) return;
  _cl_drain(cl_data);
  ADD_ZE_CL(cl_data);
}

/* Universal scheme: drain ALL cls. Used by sync APIs that don't take a
 * cmdlist argument (zeCommandQueueSynchronize, zeEventHostSynchronize,
 * zeFenceHostSynchronize). For now, a brute-force walk — O(N_cls) per
 * sync. */
static void _on_sync_drain_all(void) {
  pthread_mutex_lock(&_ze_cls_mutex);
  struct _ze_command_list_obj_data *cl_data = NULL, *tmp = NULL;
  HASH_ITER(hh, _ze_cls, cl_data, tmp) {
    _cl_drain(cl_data);
  }
  pthread_mutex_unlock(&_ze_cls_mutex);
}

static void _on_reset_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  /* Drain any slots that haven't been read yet — Reset implies the user
   * has already synchronized, so the timings are ready. The cl_data
   * entry stays in the hash; only the per-build slot list is reset. */
  _cl_drain(cl_data);
}

static void _on_destroy_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  _cl_drain(cl_data);
  _cl_slab_free(cl_data);
  free(cl_data->slots);
  free(cl_data);
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

static void THAPI_ATTRIBUTE_DESTRUCTOR _lib_cleanup() {
  if (_do_cleanup) {
    if (_do_report_injected_events)
      fprintf(stderr, "THAPI: injected events: %lu\n",
              (unsigned long)_injected_event_count);
  }
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

  s = getenv("THAPI_REPORT_INJECTED_EVENTS");
  if (s)
    _do_report_injected_events = 1;

  _do_cleanup = 1;

#ifndef THAPI_USE_DESTRUCTORS
  atexit(_lib_cleanup);
#endif
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
