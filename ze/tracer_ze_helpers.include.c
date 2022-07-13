enum _ze_obj_type {
  UNKNOWN = 0,
  DRIVER,
  DEVICE,
  COMMAND_LIST
};

struct _ze_device_obj_data {
  ze_driver_handle_t driver;
  ze_device_handle_t parent;
  ze_device_properties_t properties;
};

static int _do_profile = 0;
static int _do_chained_structs = 0;

typedef enum _ze_event_flag {
  _ZE_PROFILED = ZE_BIT(0)
} _ze_event_flag_t;
typedef _ze_event_flag_t _ze_event_flags_t;

struct _ze_event_h {
  ze_event_handle_t event;
  UT_hash_handle hh;
  ze_command_list_handle_t command_list;
  ze_event_pool_handle_t event_pool;
  ze_context_handle_t context;
  _ze_event_flags_t flags;
  /* to remember events in command lists */
  struct _ze_event_h *next, *prev;
};

typedef enum _ze_command_list_flag {
  _ZE_IMMEDIATE = ZE_BIT(0),
  _ZE_EXECUTED  = ZE_BIT(1)
} _ze_command_list_flag_t;
typedef _ze_command_list_flag_t _ze_command_list_flags_t;

struct _ze_command_list_obj_data {
  ze_device_handle_t device;
  ze_context_handle_t context;
  ze_driver_handle_t driver;
  _ze_command_list_flags_t flags;
  struct _ze_event_h *events;
};

struct _ze_obj_h {
  void *ptr;
  UT_hash_handle hh;
  enum _ze_obj_type type;
  void *obj_data;
  void (*obj_data_free)(void *obj_data);
};

struct _ze_obj_h *_ze_objs = NULL;
pthread_mutex_t _ze_objs_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void _delete_ze_obj(struct _ze_obj_h *o_h) {
  HASH_DEL(_ze_objs, o_h);
  if (o_h->obj_data && o_h->obj_data_free) {
    o_h->obj_data_free(o_h->obj_data);
  }
  free(o_h);
}

#define FIND_ZE_OBJ(key, val) do { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_FIND_PTR(_ze_objs, key, val); \
  pthread_mutex_unlock(&_ze_objs_mutex); \
} while (0)

#define ADD_ZE_OBJ(val) do { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_ADD_PTR(_ze_objs, ptr, val); \
  pthread_mutex_unlock(&_ze_objs_mutex); \
} while (0)

#define FIND_AND_DEL_ZE_OBJ(key, val) do { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_FIND_PTR(_ze_objs, key, val); \
  if (val) { \
    HASH_DEL(_ze_objs, val); \
  } \
  pthread_mutex_unlock(&_ze_objs_mutex); \
} while (0)

static inline void _register_ze_device(
 ze_device_handle_t device,
 ze_driver_handle_t driver,
 ze_device_handle_t parent) {
  struct _ze_obj_h *o_h = NULL;
  struct _ze_device_obj_data *d_data = NULL;

  FIND_ZE_OBJ(&device, o_h);
  if (o_h)
    return;

  intptr_t mem = (intptr_t)calloc(1, sizeof(struct _ze_obj_h) +
                                     sizeof(struct _ze_device_obj_data) );
  if (mem == 0)
    return;

  o_h = (struct _ze_obj_h *)mem;
  d_data = (struct _ze_device_obj_data *)(mem + sizeof(struct _ze_obj_h));

  o_h->ptr = (void *)device;
  o_h->type = DEVICE;
  d_data->driver = driver;
  d_data->parent = parent;
  o_h->obj_data = (void *)d_data;

  d_data->properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  ze_result_t res = ZE_DEVICE_GET_PROPERTIES_PTR(device, &(d_data->properties));
  if (res != ZE_RESULT_SUCCESS) {
    free((void *)mem);
    return;
  }

  ADD_ZE_OBJ(o_h);
}

static inline void _register_ze_command_list(
 ze_command_list_handle_t command_list,
 ze_context_handle_t context,
 ze_device_handle_t device,
 int immediate) {
  struct _ze_obj_h *o_h = NULL;
  struct _ze_command_list_obj_data *cl_data = NULL;
  ze_driver_handle_t driver;

  FIND_ZE_OBJ(&device, o_h);
  if (!o_h)
    return;
  driver = ((struct _ze_device_obj_data *)(o_h->obj_data))->driver;

  FIND_ZE_OBJ(&command_list, o_h);
  if (o_h)
    return;

  intptr_t mem = (intptr_t)calloc(1, sizeof(struct _ze_obj_h) +
                                     sizeof(struct _ze_command_list_obj_data) );
  if (mem == 0)
    return;

  o_h = (struct _ze_obj_h *)mem;
  cl_data = (struct _ze_command_list_obj_data *)(mem + sizeof(struct _ze_obj_h));

  o_h->ptr = (void *)command_list;
  o_h->type = COMMAND_LIST;
  cl_data->device = device;
  cl_data->context = context;
  cl_data->driver = driver;
  if (immediate)
    cl_data->flags = _ZE_IMMEDIATE;

  o_h->obj_data = (void *)cl_data;

  ADD_ZE_OBJ(o_h);
}

#define FIND_ZE_EVENT(key, val) do { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_FIND_PTR(_ze_events, key, val); \
  pthread_mutex_unlock(&_ze_events_mutex); \
} while (0)

#define ADD_ZE_EVENT(val) do { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_ADD_PTR(_ze_events, event, val); \
  pthread_mutex_unlock(&_ze_events_mutex); \
} while (0)

#define FIND_AND_DEL_ZE_EVENT(key, val) do { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_FIND_PTR(_ze_events, key, val); \
  if (val) { \
    HASH_DEL(_ze_events, val); \
  } \
  pthread_mutex_unlock(&_ze_events_mutex); \
} while (0)

static struct _ze_event_h *_ze_events = NULL;
static pthread_mutex_t _ze_events_mutex = PTHREAD_MUTEX_INITIALIZER;

struct _ze_event_pool_entry {
  ze_context_handle_t context;
  UT_hash_handle hh;
  struct _ze_event_h *events;
};

#define GET_ZE_EVENT(key, val) do { \
  struct _ze_event_pool_entry *pool = NULL; \
  pthread_mutex_lock(&_ze_event_pools_mutex); \
  HASH_FIND_PTR(_ze_event_pools, key, pool); \
  if (pool && pool->events) { \
    val = pool->events; \
    DL_DELETE(pool->events, val); \
  } else \
    val = NULL; \
  pthread_mutex_unlock(&_ze_event_pools_mutex); \
} while (0)

#define PUT_ZE_EVENT(val) do { \
  struct _ze_event_pool_entry *pool = NULL; \
  pthread_mutex_lock(&_ze_event_pools_mutex); \
  HASH_FIND_PTR(_ze_event_pools, &(val->context), pool); \
  if (!pool) { \
    pool = (struct _ze_event_pool_entry *)calloc(1, sizeof(struct _ze_event_pool_entry)); \
    if (!pool) { \
      pthread_mutex_unlock(&_ze_event_pools_mutex); \
      if (val->event_pool) { \
        if (val->event) \
          ZE_EVENT_DESTROY_PTR(val->event); \
        ZE_EVENT_POOL_DESTROY_PTR(val->event_pool); \
      } \
      free(val); \
      break; \
    } \
    pool->context = val->context; \
    HASH_ADD_PTR(_ze_event_pools, context, pool); \
  } \
  val->command_list = NULL; \
  val->flags = 0; \
  ZE_EVENT_HOST_RESET_PTR(val->event); \
  DL_PREPEND(pool->events, val); \
  pthread_mutex_unlock(&_ze_event_pools_mutex); \
} while (0)

struct _ze_event_pool_entry *_ze_event_pools = NULL;
static pthread_mutex_t _ze_event_pools_mutex = PTHREAD_MUTEX_INITIALIZER;

#define GET_ZE_EVENT_WRAPPER(val) do { \
  pthread_mutex_lock(&_ze_event_wrappers_mutex); \
  if (_ze_event_wrappers) { \
    val = _ze_event_wrappers; \
    DL_DELETE(_ze_event_wrappers, val); \
  } else { \
    val = calloc(1, sizeof(struct _ze_event_h)); \
  } \
  pthread_mutex_unlock(&_ze_event_wrappers_mutex); \
 } while (0)

#define PUT_ZE_EVENT_WRAPPER(val) do { \
  memset(val, 0, sizeof(struct _ze_event_h)); \
  pthread_mutex_lock(&_ze_event_wrappers_mutex); \
  DL_PREPEND(_ze_event_wrappers, val); \
  pthread_mutex_unlock(&_ze_event_wrappers_mutex); \
} while(0)

struct _ze_event_h *_ze_event_wrappers = NULL;
static pthread_mutex_t _ze_event_wrappers_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void _register_ze_event(
 ze_event_handle_t event,
 ze_command_list_handle_t command_list,
 struct _ze_event_h * _ze_event) {

  if (!_ze_event) {
    FIND_ZE_EVENT(&event, _ze_event);
    if (_ze_event)
      return;
    GET_ZE_EVENT_WRAPPER(_ze_event);
    if (!_ze_event)
      return;
    _ze_event->event = event;
    _ze_event->command_list = command_list;
    _ze_event->event_pool = NULL;
    _ze_event->flags = 0;
  }

  struct _ze_obj_h *o_h = NULL;
  struct _ze_command_list_obj_data *cl_data = NULL;
  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (o_h) {
    cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
    _ze_event->context = cl_data->context;
  }

  if (_do_profile && cl_data)
    DL_APPEND(cl_data->events, _ze_event);
  ADD_ZE_EVENT(_ze_event);
  if (o_h)
    ADD_ZE_OBJ(o_h);
}

static struct _ze_event_h * _get_profiling_event(
 ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;
  struct _ze_event_h *e_w;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return NULL;
  ze_context_handle_t context =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->context;
  GET_ZE_EVENT(&context, e_w);
  if (e_w) {
    e_w->command_list = command_list;
    goto cleanup;
  }

  GET_ZE_EVENT_WRAPPER(e_w);
  if (!e_w)
    goto cleanup;

  e_w->command_list = command_list;
  ze_event_pool_desc_t desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(context, &desc, 0, NULL, &e_w->event_pool);
  if (res != ZE_RESULT_SUCCESS)
    goto cleanup_wrapper;
  ze_event_desc_t e_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, NULL, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
  res = ZE_EVENT_CREATE_PTR(e_w->event_pool, &e_desc, &e_w->event);
  if (res != ZE_RESULT_SUCCESS)
    goto cleanup_ep;
  goto cleanup;
cleanup_ep:
  ZE_EVENT_POOL_DESTROY_PTR(e_w->event_pool);
cleanup_wrapper:
  PUT_ZE_EVENT_WRAPPER(e_w);
  e_w = NULL;
cleanup:
  ADD_ZE_OBJ(o_h);
  return e_w;
}

static void _profile_event_results(ze_event_handle_t event);

static inline void _unregister_ze_event(ze_event_handle_t event, int remove_cl, int get_results) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event)
    return;

  if (remove_cl) {
    struct _ze_obj_h *o_h = NULL;

    FIND_AND_DEL_ZE_OBJ(&ze_event->command_list, o_h);
    if (o_h) {
      struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
      /* Should not be necessary, just being paranoid of user having race conditions in their code */
      if (cl_data->events && ze_event->prev)
        DL_DELETE(cl_data->events, ze_event);
      if (!(cl_data->flags & _ZE_IMMEDIATE) && !(cl_data->flags & _ZE_EXECUTED))
        get_results = 0;
      ADD_ZE_OBJ(o_h);
    }
  }

  if (get_results && !(ze_event->flags & _ZE_PROFILED))
    _profile_event_results(event);
  if (ze_event->event_pool)
    PUT_ZE_EVENT(ze_event);
  else
    PUT_ZE_EVENT_WRAPPER(ze_event);
}

static inline void _dump_and_reset_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event)
    return;

  struct _ze_obj_h *o_h = NULL;
  int to_add = 0;

  pthread_mutex_lock(&_ze_objs_mutex);
  HASH_FIND_PTR(_ze_objs, &ze_event->command_list, o_h);
  if (o_h) {
    int to_profile = 0;
    struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
    if ((cl_data->flags & _ZE_IMMEDIATE) && cl_data->events && ze_event->prev)
      DL_DELETE(cl_data->events, ze_event);
    if (((cl_data->flags & _ZE_IMMEDIATE) ||
         (cl_data->flags & _ZE_EXECUTED)) &&
        !(ze_event->flags & _ZE_PROFILED))
      to_profile = 1;
    if (!(cl_data->flags & _ZE_IMMEDIATE))
      to_add = 1;
    pthread_mutex_unlock(&_ze_objs_mutex);

    if (to_profile) {
      _profile_event_results(event);
      ze_event->flags |= _ZE_PROFILED;
    }
  } else
    pthread_mutex_unlock(&_ze_objs_mutex);

  if (to_add)
    ADD_ZE_EVENT(ze_event);
  else
    PUT_ZE_EVENT_WRAPPER(ze_event);
}

static inline void _dump_and_reset_our_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event)
    return;

  if (ze_event->event_pool) { /* one of ours */
    /* dump events that are ours, the other should have been reset by the user */
    _profile_event_results(event);
    ZE_EVENT_HOST_RESET_PTR(event);
  }
  ze_event->flags &= ~_ZE_PROFILED;
  ADD_ZE_EVENT(ze_event);
}

static void _profile_event_results(ze_event_handle_t event) {
  ze_kernel_timestamp_result_t res = {0};
  ze_result_t status;
  ze_result_t timestamp_status;

  if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results)) {
    status = ZE_EVENT_QUERY_STATUS_PTR(event);
    timestamp_status = ZE_EVENT_QUERY_KERNEL_TIMESTAMP_PTR(event, &res);
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results,
                  event, status, timestamp_status,
                  res.global.kernelStart,
                  res.global.kernelEnd,
                  res.context.kernelStart,
                  res.context.kernelEnd);
  }
}

static void _event_cleanup() {
  struct _ze_event_h *ze_event = NULL;
  struct _ze_event_h *tmp = NULL;

  HASH_ITER(hh, _ze_events, ze_event, tmp) {
    HASH_DEL(_ze_events, ze_event);
    if (ze_event->event && !(ze_event->flags & _ZE_PROFILED))
      _profile_event_results(ze_event->event);
    if (ze_event->event_pool) {
      if (ze_event->event)
        ZE_EVENT_DESTROY_PTR(ze_event->event);
      ZE_EVENT_POOL_DESTROY_PTR(ze_event->event_pool);
    }
    free(ze_event);
  }
}

static void _context_cleanup(ze_context_handle_t context){
  struct _ze_event_h *ze_event = NULL;
  struct _ze_event_h *tmp = NULL;
  pthread_mutex_lock(&_ze_events_mutex);
  HASH_ITER(hh, _ze_events, ze_event, tmp) {
    if (ze_event->context == context) {
      HASH_DEL(_ze_events, ze_event);
      if (ze_event->event && !(ze_event->flags & _ZE_PROFILED))
        _profile_event_results(ze_event->event);
      if (ze_event->event_pool) {
        if (ze_event->event)
          ZE_EVENT_DESTROY_PTR(ze_event->event);
        ZE_EVENT_POOL_DESTROY_PTR(ze_event->event_pool);
      }
      free(ze_event);
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
      ZE_EVENT_DESTROY_PTR(elt->event);
      ZE_EVENT_POOL_DESTROY_PTR(elt->event_pool);
      PUT_ZE_EVENT_WRAPPER(elt);
    }
    free(pool);
  }
  pthread_mutex_unlock(&_ze_event_pools_mutex);
}

static void _reset_ze_command_list(ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return;
  struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
  struct _ze_event_h *elt = NULL, *tmp = NULL;
  DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
    DL_DELETE(cl_data->events, elt);
    _unregister_ze_event(elt->event, 0, cl_data->flags & _ZE_EXECUTED);
  }
  cl_data->flags &= ~_ZE_EXECUTED;
  ADD_ZE_OBJ(o_h);
}

static void _execute_ze_command_lists(uint32_t numCommandLists, ze_command_list_handle_t *phCommandLists) {
  for (uint32_t i = 0; i < numCommandLists; i++) {
    struct _ze_obj_h *o_h = NULL;
    FIND_AND_DEL_ZE_OBJ(phCommandLists + i, o_h);
    if (o_h) {
      struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
      /* dump events if they were executed */
      if (cl_data->flags & _ZE_EXECUTED) {
        struct _ze_event_h *elt = NULL;
        DL_FOREACH(cl_data->events, elt) {
          _dump_and_reset_our_event(elt->event);
        }
      } else
        cl_data->flags |= _ZE_EXECUTED;
      ADD_ZE_OBJ(o_h);
    }
  }
}

static void _unregister_ze_command_list(ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return;
  if (_do_profile) {
    struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
    struct _ze_event_h *elt = NULL, *tmp = NULL;
    DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
      DL_DELETE(cl_data->events, elt);
      _unregister_ze_event(elt->event, 0, (cl_data->flags & _ZE_IMMEDIATE) || (cl_data->flags & _ZE_EXECUTED));
    }
  }
  free(o_h);
}

static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile unsigned int _initialized = 0;
static int _paranoid_drift = 0;

static inline int _do_state() {
  return _do_profile ||
         tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
         tracepoint_enabled(lttng_ust_ze_properties, memory_info_range);
}

static void _lib_cleanup() {
  if (_do_profile) {
    _event_cleanup();
  }
}

static void _dump_driver_subdevice_properties(ze_driver_handle_t hDriver, ze_device_handle_t hDevice) {
  if (!tracepoint_enabled(lttng_ust_ze_properties, subdevice))
    return;

  uint32_t subDeviceCount = 0;
  if (ZE_DEVICE_GET_SUB_DEVICES_PTR(hDevice, &subDeviceCount, NULL ) != ZE_RESULT_SUCCESS || subDeviceCount == 0)
    return;
  ze_device_handle_t* phSubDevices = (ze_device_handle_t*) alloca(subDeviceCount * sizeof(ze_device_handle_t));

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
  if (ZE_DEVICE_GET_GLOBAL_TIMESTAMPS_PTR(hDevice, &hostTimestamp, &deviceTimestamp) == ZE_RESULT_SUCCESS)
    do_tracepoint(lttng_ust_ze_properties, device_timer, hDevice, hostTimestamp, deviceTimestamp);
}

static void _dump_command_list_device_timer(ze_command_list_handle_t hCommandList) {
  struct _ze_obj_h *o_h = NULL;
  FIND_ZE_OBJ(&hCommandList, o_h);
  if (o_h) {
    ze_device_handle_t hDevice = ((struct _ze_command_list_obj_data *)(o_h->obj_data))->device;
    _dump_device_timer(hDevice);
  }
}

static void _dump_driver_device_properties(ze_driver_handle_t hDriver) {
  uint32_t deviceCount = 0;
  if (ZE_DEVICE_GET_PTR(hDriver, &deviceCount, NULL) != ZE_RESULT_SUCCESS || deviceCount == 0)
    return;
  ze_device_handle_t* phDevices = (ze_device_handle_t*)alloca(deviceCount * sizeof(ze_device_handle_t));

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
  kernelProperties.stype=ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  kernelProperties.pNext=NULL;
  if (ZE_KERNEL_GET_PROPERTIES_PTR(hKernel, &kernelProperties) == ZE_RESULT_SUCCESS)
    tracepoint(lttng_ust_ze_properties, kernel, hKernel, &kernelProperties);
}

static void _dump_properties() {
  uint32_t driverCount = 0;
  if(ZE_DRIVER_GET_PTR(&driverCount, NULL) != ZE_RESULT_SUCCESS || driverCount == 0)
    return;
  ze_driver_handle_t* phDrivers = (ze_driver_handle_t*)alloca(driverCount * sizeof(ze_driver_handle_t));
  if(ZE_DRIVER_GET_PTR(&driverCount, phDrivers) != ZE_RESULT_SUCCESS)
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
  size_t       size;
  char        *buildLog;
  ze_result_t  res;

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
    if (ZE_MEM_GET_ALLOC_PROPERTIES_PTR(hContext, ptr, &memAllocProperties, &hDevice) == ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, memory_info_properties, hContext, ptr, &memAllocProperties, hDevice);
  }
  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
    void *base = NULL;
    size_t size = 0;
    if (ZE_MEM_GET_ADDRESS_RANGE_PTR(hContext, ptr, &base, &size) == ZE_RESULT_SUCCESS)
      do_tracepoint(lttng_ust_ze_properties, memory_info_range, hContext, ptr, base, size);
  }
}

static inline void _dump_memory_info(ze_command_list_handle_t hCommandList, const void *ptr) {
  struct _ze_obj_h *o_h = NULL;
  FIND_ZE_OBJ(&hCommandList, o_h);
  if (o_h) {
    ze_context_handle_t hContext = ((struct _ze_command_list_obj_data *)(o_h->obj_data))->context;
    _dump_memory_info_ctx(hContext, ptr);
  }
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
    void* ptr = dlsym(handle, "zeInit");
    if (ptr == (void*)&zeInit) { //opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if( !handle ) {
    fprintf(stderr, "Failure: could not load ze library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_ZE_VERBOSE");
  if (s)
    verbose = 1;

  s = getenv("LTTNG_UST_ZE_CHAINED_STRUCTS");
  if (s)
    _do_chained_structs = 1;

  find_ze_symbols(handle, verbose);

  //FIX for intel tracing layer that needs to register its callbacks first...
  ZE_INIT_PTR(0);

  if (tracepoint_enabled(lttng_ust_ze_properties, driver) || tracepoint_enabled(lttng_ust_ze_properties, device) || tracepoint_enabled(lttng_ust_ze_properties, subdevice))
    _dump_properties();
  s = getenv("LTTNG_UST_ZE_PROFILE");
  if (s)
    _do_profile = 1;
  s = getenv("LTTNG_UST_ZE_PARANOID_DRIFT");
  if (s) {
    if (_do_profile)
      _paranoid_drift = 1;
    else if (verbose)
      fprintf(stderr, "Warning: LTTNG_UST_ZE_PARANOID_DRIFT not activated without LTTNG_UST_ZE_PROFILE\n");
  }
  if (_do_profile)
    atexit(&_lib_cleanup);
}

static inline void _init_tracer(void) {
  if( __builtin_expect (_initialized, 1) )
    return;
  /* Avoid reentrancy */
  if (!in_init) {
    in_init=1;
    __sync_synchronize();
    pthread_once(&_init, _load_tracer);
    __sync_synchronize();
    in_init=0;
  }
  _initialized = 1;
}
