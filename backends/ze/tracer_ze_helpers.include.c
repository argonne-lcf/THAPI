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

typedef enum _ze_command_list_flag { _ZE_EXECUTED = ZE_BIT(0) } _ze_command_list_flag_t;
typedef _ze_command_list_flag_t _ze_command_list_flags_t;

struct _ze_event_h;

struct _ze_command_list_obj_data {
  void *ptr; /* the ze_command_list_handle_t this entry tracks */
  UT_hash_handle hh;
  _ze_command_list_flags_t flags;
  struct _ze_event_h *events;
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

static inline void _on_create_command_list(ze_command_list_handle_t command_list, int immediate) {
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
  /* Immediate cls have no Execute step; their appends run on the device the
   * moment they're submitted. Treat them as already-executed so drainers
   * (Reset/Destroy hooks) query their events via _ZE_EXECUTED uniformly. */
  if (immediate)
    cl_data->flags = _ZE_EXECUTED;

  ADD_ZE_CL(cl_data);
}

typedef enum _ze_event_flag { _ZE_IMMEDIATE_CMD = ZE_BIT(0) } _ze_event_flag_t;
typedef _ze_event_flag_t _ze_event_flags_t;

struct _ze_event_h {
  ze_event_handle_t event;
  UT_hash_handle hh;
  ze_event_pool_handle_t event_pool;
  ze_context_handle_t context;
  _ze_event_flags_t flags;
  /* to remember events in command lists */
  struct _ze_event_h *next, *prev;
};

static struct _ze_event_h *_ze_events = NULL;
static pthread_mutex_t _ze_events_mutex = PTHREAD_MUTEX_INITIALIZER;

#define FIND_ZE_EVENT(key, val)                                                                    \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_events_mutex);                                                         \
    HASH_FIND_PTR(_ze_events, key, val);                                                           \
    pthread_mutex_unlock(&_ze_events_mutex);                                                       \
  } while (0)

#define ADD_ZE_EVENT(val)                                                                          \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_events_mutex);                                                         \
    HASH_ADD_PTR(_ze_events, event, val);                                                          \
    pthread_mutex_unlock(&_ze_events_mutex);                                                       \
  } while (0)

#define FIND_AND_DEL_ZE_EVENT(key, val)                                                            \
  do {                                                                                             \
    pthread_mutex_lock(&_ze_events_mutex);                                                         \
    HASH_FIND_PTR(_ze_events, key, val);                                                           \
    if (val) {                                                                                     \
      HASH_DEL(_ze_events, val);                                                                   \
    }                                                                                              \
    pthread_mutex_unlock(&_ze_events_mutex);                                                       \
  } while (0)

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
    val->flags = 0;                                                                                \
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

/* Snapshot context + immediate-flag from cmdlist into the event wrapper.
 * The immediate flag is read at register time (not at _on_reset_event
 * time) because by reset time the cmdlist may already be destroyed and
 * zeCommandListIsImmediate would dereference a freed handle. */
static inline void _tag_event_from_cl(struct _ze_event_h *_ze_event,
                                      ze_command_list_handle_t command_list) {
  ze_context_handle_t context = NULL;
  ze_result_t res = ZE_COMMAND_LIST_GET_CONTEXT_HANDLE_PTR(command_list, &context);
  if (res == ZE_RESULT_SUCCESS && context)
    _ze_event->context = context;
  else
    THAPI_DBGLOG("zeCommandListGetContextHandle failed with %d for command list: %p", res,
                 command_list);

  ze_bool_t is_immediate = 0;
  if (ZE_COMMAND_LIST_IS_IMMEDIATE_PTR(command_list, &is_immediate) == ZE_RESULT_SUCCESS &&
      is_immediate)
    _ze_event->flags |= _ZE_IMMEDIATE_CMD;
}

/* Append an event wrapper we own to its cmdlist's events list, under the
 * cl-hash lock (the FIND_AND_DEL/ADD pattern guards cl_data against a
 * concurrent free in _on_destroy_command_list). */
static inline void _attach_event_to_cl(struct _ze_event_h *_ze_event,
                                       ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    THAPI_DBGLOG("Could not get command list associated to event: %p", _ze_event->event);
    return;
  }
  DL_APPEND(cl_data->events, _ze_event);
  ADD_ZE_CL(cl_data);
}

/* Register an injected (tracer-owned) event. Caller has already populated
 * _ze_event->event and _ze_event->event_pool via _get_profiling_event. */
static inline void _register_our_event(struct _ze_event_h *_ze_event,
                                       ze_command_list_handle_t command_list) {
  _tag_event_from_cl(_ze_event, command_list);
  _attach_event_to_cl(_ze_event, command_list);
  ADD_ZE_EVENT(_ze_event);
}

/* Register a user event (we don't own its lifetime). Look up or create the
 * wrapper; users are responsible for reset/destroy, so we don't attach it
 * to the cl's events list. */
static inline void _register_user_event(ze_event_handle_t event,
                                        ze_command_list_handle_t command_list) {
  struct _ze_event_h *_ze_event = NULL;
  FIND_ZE_EVENT(&event, _ze_event);
  if (_ze_event)
    return; /* already tracked, nothing more to do */

  GET_ZE_EVENT_WRAPPER(_ze_event);
  if (!_ze_event) {
    THAPI_DBGLOG("Could not get event wrapper for: %p", event);
    return;
  }
  /* GET_ZE_EVENT_WRAPPER returns a fully-zeroed wrapper (calloc on first use,
   * memset by PUT_ZE_EVENT_WRAPPER on recycle), so event_pool and flags are
   * already 0 — only set the fields we actually want non-zero. */
  _ze_event->event = event;

  _tag_event_from_cl(_ze_event, command_list);
  ADD_ZE_EVENT(_ze_event);
}

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

static void _profile_event_results(ze_event_handle_t event) {
  ze_kernel_timestamp_result_t res = {0};
  ze_result_t status;
  ze_result_t timestamp_status;

  if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results)) {
    status = ZE_EVENT_QUERY_STATUS_PTR(event);
    timestamp_status = ZE_EVENT_QUERY_KERNEL_TIMESTAMP_PTR(event, &res);
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results, event, status, timestamp_status,
                  res.global.kernelStart, res.global.kernelEnd, res.context.kernelStart,
                  res.context.kernelEnd);
  }
}

static inline void _on_destroy_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event) {
    return;
  }

  _profile_event_results(event);
  PUT_ZE_EVENT_WRAPPER(ze_event);
}

/* Caller already holds the wrapper (e.g. iterating cl_data->events) and
 * has removed it from any per-cl list. Drops it from the global events
 * hash, optionally emits its timestamp tracepoint, and recycles. */
static inline void _unregister_ze_event(struct _ze_event_h *ze_event, int get_results) {
  struct _ze_event_h *evicted = NULL;
  FIND_AND_DEL_ZE_EVENT(&ze_event->event, evicted);
  /* evicted should be == ze_event; if not, our hash bookkeeping is corrupt. */

  if (get_results)
    _profile_event_results(ze_event->event);
  if (ze_event->event_pool)
    PUT_ZE_EVENT(ze_event);
  else
    PUT_ZE_EVENT_WRAPPER(ze_event);
}

static inline void _on_reset_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event) {
    THAPI_DBGLOG("Could not find event: %p", event);
    return;
  }

  _profile_event_results(event);

  if (!(ze_event->flags & _ZE_IMMEDIATE_CMD))
    ADD_ZE_EVENT(ze_event);
  else
    PUT_ZE_EVENT_WRAPPER(ze_event);
}

static inline void _dump_and_reset_our_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event) {
    THAPI_DBGLOG("Could not find event: %p", event);
    return;
  }

  _profile_event_results(event);
  ZE_EVENT_HOST_RESET_PTR(event);
  ADD_ZE_EVENT(ze_event);
}

/* Tear down a wrapper: optionally emit its timestamp tracepoint, then
 * destroy the injected event+pool if we own them, then recycle the
 * wrapper. Caller must have already removed it from any list/hash that
 * references it. */
static inline void _dispose_event_wrapper(struct _ze_event_h *ze_event, int do_dump) {
  if (do_dump && ze_event->event)
    _profile_event_results(ze_event->event);
  if (ze_event->event_pool) {
    if (ze_event->event)
      ZE_EVENT_DESTROY_PTR(ze_event->event);
    ZE_EVENT_POOL_DESTROY_PTR(ze_event->event_pool);
  }
  PUT_ZE_EVENT_WRAPPER(ze_event);
}

static void _event_cleanup() {
  struct _ze_event_h *ze_event = NULL;
  struct _ze_event_h *tmp = NULL;
  HASH_ITER(hh, _ze_events, ze_event, tmp) {
    HASH_DEL(_ze_events, ze_event);
    _dispose_event_wrapper(ze_event, 1);
  }
}

static void _on_destroy_context(ze_context_handle_t context) {
  struct _ze_event_h *ze_event = NULL;
  struct _ze_event_h *tmp = NULL;
  pthread_mutex_lock(&_ze_events_mutex);
  HASH_ITER(hh, _ze_events, ze_event, tmp) {
    if (ze_event->context == context) {
      HASH_DEL(_ze_events, ze_event);
      _dispose_event_wrapper(ze_event, 1);
    }
  }
  pthread_mutex_unlock(&_ze_events_mutex);
  pthread_mutex_lock(&_ze_event_pools_mutex);
  struct _ze_event_pool_entry *pool = NULL;
  HASH_FIND_PTR(_ze_event_pools, &context, pool);
  if (pool) {
    HASH_DEL(_ze_event_pools, pool);
    struct _ze_event_h *elt = NULL, *tmp = NULL;
    DL_FOREACH_SAFE(pool->events, elt, tmp) {
      DL_DELETE(pool->events, elt);
      /* Wrapper is in the free list — its event was already dumped+reset
       * by whoever recycled it. Don't dump again, just tear down. */
      _dispose_event_wrapper(elt, 0);
    }
    free(pool);
  }
  pthread_mutex_unlock(&_ze_event_pools_mutex);
}

static void _on_reset_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;

  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  struct _ze_event_h *elt = NULL, *tmp = NULL;
  DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
    DL_DELETE(cl_data->events, elt);
    _unregister_ze_event(elt, cl_data->flags & _ZE_EXECUTED);
  }
  cl_data->flags &= ~_ZE_EXECUTED;
  ADD_ZE_CL(cl_data);
}

static void _on_execute_command_lists(uint32_t numCommandLists,
                                      ze_command_list_handle_t *phCommandLists) {
  for (uint32_t i = 0; i < numCommandLists; i++) {
    struct _ze_command_list_obj_data *cl_data = NULL;
    FIND_AND_DEL_ZE_CL(phCommandLists + i, cl_data);
    if (cl_data) {
      /* dump events if they were executed */
      if (cl_data->flags & _ZE_EXECUTED) {
        struct _ze_event_h *elt = NULL;
        DL_FOREACH(cl_data->events, elt) { _dump_and_reset_our_event(elt->event); }
      } else
        cl_data->flags |= _ZE_EXECUTED;
      ADD_ZE_CL(cl_data);
    } else
      THAPI_DBGLOG("Could not get command list: %p", phCommandLists[i]);
  }
}

static void _on_destroy_command_list(ze_command_list_handle_t command_list) {
  struct _ze_command_list_obj_data *cl_data = NULL;

  FIND_AND_DEL_ZE_CL(&command_list, cl_data);
  if (!cl_data) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  if (_do_profile) {
    struct _ze_event_h *elt = NULL, *tmp = NULL;
    DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
      DL_DELETE(cl_data->events, elt);
      _unregister_ze_event(elt, cl_data->flags & _ZE_EXECUTED);
    }
  }
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
    if (_do_profile)
      _event_cleanup();
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
