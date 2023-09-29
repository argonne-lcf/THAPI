#include "thapi_sampling.h"

#ifdef THAPI_DEBUG
#define TAHPI_LOG stderr
#define THAPI_DBGLOG(fmt, ...) \
 do { \
   fprintf(TAHPI_LOG, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__, __VA_ARGS__); \
 } while(0)
#define THAPI_DBGLOG_NO_ARGS(fmt) \
 do { \
   fprintf(TAHPI_LOG, "THAPI(%s:%d): " fmt "\n", __func__, __LINE__); \
 } while(0)
#else
#define THAPI_DBGLOG(...) do { } while (0)
#define THAPI_DBGLOG_NO_ARGS(fmt) do { } while (0)
#endif

enum _ze_obj_type {
  UNKNOWN = 0,
  DRIVER,
  DEVICE,
  COMMAND_LIST,
  EVENT
};

struct _ze_device_obj_data {
  ze_driver_handle_t driver;
  ze_device_handle_t parent;
  ze_device_properties_t properties;
};

static int _do_profile = 0;
static int _do_chained_structs = 0;

typedef enum _ze_command_list_flag {
  _ZE_IMMEDIATE = ZE_BIT(0),
  _ZE_EXECUTED  = ZE_BIT(1)
} _ze_command_list_flag_t;
typedef _ze_command_list_flag_t _ze_command_list_flags_t;

struct _ze_event_h;

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

/*
static inline void _delete_ze_obj(struct _ze_obj_h *o_h) {
  HASH_DEL(_ze_objs, o_h);
  if (o_h->obj_data && o_h->obj_data_free) {
    o_h->obj_data_free(o_h->obj_data);
  }
  free(o_h);
}
*/

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
  if (o_h) {
    THAPI_DBGLOG("Device already registered: %p", device);
    return;
  }

  intptr_t mem = (intptr_t)calloc(1, sizeof(struct _ze_obj_h) +
                                     sizeof(struct _ze_device_obj_data) );
  if (mem == 0) {
    THAPI_DBGLOG_NO_ARGS("Failed to allocate memory");
    return;
  }

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

static inline void _on_create_command_list(
 ze_command_list_handle_t command_list,
 ze_context_handle_t context,
 ze_device_handle_t device,
 int immediate) {
  struct _ze_obj_h *o_h = NULL;
  struct _ze_command_list_obj_data *cl_data = NULL;
  ze_driver_handle_t driver;

  FIND_ZE_OBJ(&device, o_h);
  if (!o_h) {
    THAPI_DBGLOG("Could not find device: %p associated with command list: %p", device, command_list);
    return;
  }
  driver = ((struct _ze_device_obj_data *)(o_h->obj_data))->driver;

  FIND_ZE_OBJ(&command_list, o_h);
  if (o_h) {
    THAPI_DBGLOG("Command list already registered: %p", command_list);
    return;
  }

  intptr_t mem = (intptr_t)calloc(1, sizeof(struct _ze_obj_h) +
                                     sizeof(struct _ze_command_list_obj_data) );
  if (mem == 0) {
    THAPI_DBGLOG_NO_ARGS("Failed to allocate memory");
    return;
  }

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

typedef enum _ze_event_flag {
  _ZE_PROFILED = ZE_BIT(0),
  _ZE_IMMEDIATE_CMD = ZE_BIT(1)
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

static struct _ze_event_h *_ze_events = NULL;
static pthread_mutex_t _ze_events_mutex = PTHREAD_MUTEX_INITIALIZER;

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

struct _ze_event_pool_entry {
  ze_context_handle_t context;
  UT_hash_handle hh;
  struct _ze_event_h *events;
};

struct _ze_event_pool_entry *_ze_event_pools = NULL;
static pthread_mutex_t _ze_event_pools_mutex = PTHREAD_MUTEX_INITIALIZER;

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
      THAPI_DBGLOG_NO_ARGS("Failed to allocate memory"); \
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

struct _ze_event_h *_ze_event_wrappers = NULL;
static pthread_mutex_t _ze_event_wrappers_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static inline void _register_ze_event(
 ze_event_handle_t event,
 ze_command_list_handle_t command_list,
 struct _ze_event_h * _ze_event) {
  // If _ze_event, our event
  if (!_ze_event) {
    FIND_ZE_EVENT(&event, _ze_event);
    if (_ze_event) {
      if (_ze_event->flags & _ZE_IMMEDIATE_CMD) {
        THAPI_DBGLOG("Event already registered: %p", event);
      }
      _ze_event->command_list = command_list;
      return;
    }

    GET_ZE_EVENT_WRAPPER(_ze_event);
    if (!_ze_event) {
      THAPI_DBGLOG("Could not get event wrapper for: %p", event);
      return;
    }
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
    if (cl_data->flags & _ZE_IMMEDIATE)
      _ze_event->flags |= _ZE_IMMEDIATE_CMD;
  } else
    THAPI_DBGLOG("Could not get command list associated to event: %p", event);

  /* only track our events, users are responsible for reseting/deleting their events */
  if (cl_data && _ze_event->event_pool)
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
  if (!o_h) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return NULL;
  }
  ze_context_handle_t context =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->context;
  GET_ZE_EVENT(&context, e_w);
  if (e_w) {
    e_w->command_list = command_list;
    goto cleanup;
  }

  GET_ZE_EVENT_WRAPPER(e_w);
  if (!e_w) {
    THAPI_DBGLOG("Could not create a new event wrapper for command list: %p", command_list);
    goto cleanup;
  }

  e_w->command_list = command_list;
  ze_event_pool_desc_t desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(context, &desc, 0, NULL, &e_w->event_pool);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventPoolCreate failed with %d, for command list: %p, context: %p", res, command_list, context);
    goto cleanup_wrapper;
  }
  ze_event_desc_t e_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, NULL, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
  res = ZE_EVENT_CREATE_PTR(e_w->event_pool, &e_desc, &e_w->event);
  if (res != ZE_RESULT_SUCCESS) {
    THAPI_DBGLOG("zeEventCreate failed with %d, for event pool: %p, command list: %p, context: %p", res, e_w->event_pool, command_list, context);
    goto cleanup_ep;
  }
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

static inline void _on_created_event(ze_event_handle_t event) {
#ifdef THAPI_DEBUG
  struct _ze_obj_h *o_h = NULL;
  FIND_ZE_OBJ(&event, o_h);
  if (o_h) {
    THAPI_DBGLOG("Event already registered: %p", event);
    return;
  }

  intptr_t mem = (intptr_t)calloc(1, sizeof(struct _ze_obj_h));
  if (mem == 0) {
    THAPI_DBGLOG_NO_ARGS("Failed to allocate memory");
    return;
  }

  o_h = (struct _ze_obj_h *)mem;
  o_h->ptr = (void *)event;
  o_h->type = EVENT;

  ADD_ZE_OBJ(o_h);
#else
  (void)event;
#endif
}

static inline void _on_destroy_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

#ifdef THAPI_DEBUG
  struct _ze_obj_h *o_h = NULL;
  FIND_AND_DEL_ZE_OBJ(&event, o_h);
  if (!o_h) {
    THAPI_DBGLOG("Could not find event: %p", event);
  }
#endif

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event) {
    return;
  }

  if (!(ze_event->flags & _ZE_PROFILED))
    _profile_event_results(event);
  PUT_ZE_EVENT_WRAPPER(ze_event);
}

static inline void _unregister_ze_event(ze_event_handle_t event, int get_results) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event) {
    THAPI_DBGLOG("Could not find event: %p", event);
    return;
  }

  if (get_results && !(ze_event->flags & _ZE_PROFILED))
    _profile_event_results(event);
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

  if (!(ze_event->flags & _ZE_PROFILED))
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

static void _on_destroy_context(ze_context_handle_t context){
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
       /* should put? */
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

static void _on_reset_command_list(ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
  struct _ze_event_h *elt = NULL, *tmp = NULL;
  DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
    DL_DELETE(cl_data->events, elt);
    _unregister_ze_event(elt->event, cl_data->flags & _ZE_EXECUTED);
  }
  cl_data->flags &= ~_ZE_EXECUTED;
  ADD_ZE_OBJ(o_h);
}

static void _on_execute_command_lists(uint32_t numCommandLists, ze_command_list_handle_t *phCommandLists) {
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
    } else
      THAPI_DBGLOG("Could not get command list: %p", phCommandLists[i]);
  }
}

static void _on_destroy_command_list(ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h) {
    THAPI_DBGLOG("Could not get command list: %p", command_list);
    return;
  }
  if (_do_profile) {
    struct _ze_command_list_obj_data *cl_data = (struct _ze_command_list_obj_data *)(o_h->obj_data);
    struct _ze_event_h *elt = NULL, *tmp = NULL;
    DL_FOREACH_SAFE(cl_data->events, elt, tmp) {
      DL_DELETE(cl_data->events, elt);
      _unregister_ze_event(elt->event, (cl_data->flags & _ZE_IMMEDIATE) || (cl_data->flags & _ZE_EXECUTED));
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

////////////////////////////////////////////
#define _ZE_ERROR_MSG(NAME,RES) {fprintf(stderr,"%s() failed at %d(%s): res=%x\n",(NAME),__LINE__,__FILE__,(RES));}
#define _ZE_ERROR_MSG_NOTERMINATE(NAME,RES) {fprintf(stderr,"%s() error at %d(%s): res=%x\n",(NAME),__LINE__,__FILE__,(RES));}
#define _ERROR_MSG(MSG) {perror((MSG)); fprintf(stderr,"errno=%d at %d(%s)",errno,__LINE__,__FILE__);}

static int _sampling_initialized = 0;
// Static handles to stay throughout the execution
static ze_device_handle_t* _sampling_hDevices;
static zes_freq_handle_t** _sampling_hFrequencies;
static zes_pwr_handle_t** _sampling_hPowers;
static zes_engine_handle_t** _sampling_engineHandles;
static uint32_t _sampling_deviceCount = 0;
static uint32_t _sampling_subDeviceCount = 0;
static uint32_t* _sampling_freqDomainCounts;
static uint32_t* _sampling_powerDomainCounts;
static uint32_t* _sampling_engineCounts;

typedef struct {
    uint64_t timestamp;
    uint64_t computeActive;
} computeEngineData;

typedef struct {
    uint64_t timestamp;
    uint64_t copyActive;
} copyEngineData;

int initializeHandles() {
  ze_result_t res;

  const char *e = getenv("ZES_ENABLE_SYSMAN");
  if (!(e && e[0] == '1'))  {
    fprintf(stderr,"ZES_ENABLE_SYSMAN needs to be set!\n");
    return -1;
  }
#ifdef CALL_ZEINIT
  res = zeInit(ZE_INIT_FLAG_GPU_ONLY);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("zeInit", res);
    return -1;
  }
#endif

  // Query driver
  uint32_t driverCount = 0;
  res = ZE_DRIVER_GET_PTR(&driverCount, NULL);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("1st ZE_DRIVER_GET_PTR", res);
    return -1;
  }

  ze_driver_handle_t *hDriver = (ze_driver_handle_t*) malloc(driverCount * sizeof(ze_driver_handle_t));
  res = ZE_DRIVER_GET_PTR(&driverCount, hDriver);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("2nd ZE_DRIVER_GET_PTR", res);
    return -1;
  }

  // Query device count
  res = ZE_DEVICE_GET_PTR(hDriver[0], &_sampling_deviceCount, NULL);
  if (res != ZE_RESULT_SUCCESS || _sampling_deviceCount == 0) {
    fprintf(stderr, "ERROR: No device found!\n");
    _ZE_ERROR_MSG("ZE_DEVICE_GET_PTR", res);
    return -1;
  }

  _sampling_hDevices = (ze_device_handle_t*) malloc(_sampling_deviceCount * sizeof(ze_device_handle_t));
  res = ZE_DEVICE_GET_PTR(hDriver[0], &_sampling_deviceCount, _sampling_hDevices);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("2nd ZE_DRIVER_GET_PTR", res);
    return -1;
  }
  //Get no sub-devices
  zeDeviceGetSubDevices(_sampling_hDevices[0], &_sampling_subDeviceCount, NULL);

  _sampling_hFrequencies = (zes_freq_handle_t**) malloc(_sampling_deviceCount * sizeof(zes_freq_handle_t*));
  _sampling_freqDomainCounts = (uint32_t*) malloc(_sampling_deviceCount * sizeof(uint32_t));

  _sampling_hPowers = (zes_pwr_handle_t**) malloc(_sampling_deviceCount * sizeof(zes_pwr_handle_t*));
  _sampling_powerDomainCounts = (uint32_t*) malloc(_sampling_deviceCount * sizeof(uint32_t));

  _sampling_engineHandles = (zes_engine_handle_t**) malloc(_sampling_deviceCount * sizeof(zes_engine_handle_t*));
  _sampling_engineCounts = (uint32_t*) malloc(_sampling_deviceCount * sizeof(uint32_t));

  for (uint32_t i = 0; i < _sampling_deviceCount; i++) {
    // Get frequency domains for each device
    res = zesDeviceEnumFrequencyDomains(_sampling_hDevices[i], &_sampling_freqDomainCounts[i], NULL);
    if (res != ZE_RESULT_SUCCESS) {
      printf("zesDeviceEnumFrequencyDomains (count query) failed for device %d: %d\n", i, res);
      return(-1);
    }

    _sampling_hFrequencies[i] = (zes_freq_handle_t*) malloc(_sampling_freqDomainCounts[i] * sizeof(zes_freq_handle_t));
    res = zesDeviceEnumFrequencyDomains(_sampling_hDevices[i], &_sampling_freqDomainCounts[i], _sampling_hFrequencies[i]);
    if (res != ZE_RESULT_SUCCESS) {
      printf("zesDeviceEnumFrequencyDomains failed for device %d: %d\n", i, res);
      return(-1);
    }
    // Get power domains for each device
    res = zesDeviceEnumPowerDomains(_sampling_hDevices[i], &_sampling_powerDomainCounts[i], NULL);
    if (res != ZE_RESULT_SUCCESS) {
      printf("zesDeviceEnumPowerDomains (count query) failed for device %d: %d\n", i, res);
      return(-1);
    }

    _sampling_hPowers[i] = (zes_pwr_handle_t*) malloc(_sampling_powerDomainCounts[i] * sizeof(zes_pwr_handle_t));
    res = zesDeviceEnumPowerDomains(_sampling_hDevices[i], &_sampling_powerDomainCounts[i], _sampling_hPowers[i]);
    if (res != ZE_RESULT_SUCCESS) {
      printf("zesDeviceEnumPowerDomains failed for device %d: %d\n", i, res);
      return(-1);
    }
    // Get the available engines for each device
    res = zesDeviceEnumEngineGroups(_sampling_hDevices[i], &_sampling_engineCounts[i], NULL);
    if (res != ZE_RESULT_SUCCESS || _sampling_engineCounts[i] == 0) {
      printf("No engine groups found\n");
      return(-1);
    }
    
    _sampling_engineHandles[i] = (zes_engine_handle_t*)malloc(_sampling_engineCounts[i] * sizeof(zes_engine_handle_t));
    res = zesDeviceEnumEngineGroups(_sampling_hDevices[i], &_sampling_engineCounts[i], _sampling_engineHandles[i]);
    if (res != ZE_RESULT_SUCCESS) {
      printf("Failed to get engine group handles\n");
      free(_sampling_engineHandles);
      return (-1);
    }
  }
  free(hDriver);
  _sampling_initialized=1;
  return 0;
}

void readFrequency(uint32_t deviceIdx, uint32_t domainIdx, uint32_t *frequency) {
  if (!_sampling_initialized) return;
  *frequency=0;
  zes_freq_state_t freqState;
  if (zesFrequencyGetState(_sampling_hFrequencies[deviceIdx][domainIdx], &freqState) == ZE_RESULT_SUCCESS) {
    *frequency = freqState.actual;
  }
}

void readEnergy(uint32_t deviceIdx, uint32_t domainIdx, uint64_t *ts_us, uint64_t *energy_uj) {
  if (!_sampling_initialized) return;
  *ts_us = 0;
  *energy_uj = 0;
  zes_power_energy_counter_t energyCounter;
  if (zesPowerGetEnergyCounter(_sampling_hPowers[deviceIdx][domainIdx], &energyCounter) == ZE_RESULT_SUCCESS) {
    *ts_us = energyCounter.timestamp;
    *energy_uj = energyCounter.energy;
  }
}

void readComputeE(uint32_t deviceIdx, computeEngineData *computeData ){
  ze_result_t result;
  for (uint32_t i = 0; i < _sampling_subDeviceCount; i++) {
    computeData[i].computeActive = 0;
    computeData[i].timestamp = 0;
  }
  for (uint32_t j = 0; j < _sampling_engineCounts[deviceIdx]; ++j) {
    zes_engine_properties_t engineProp = {};
    result = zesEngineGetProperties(_sampling_engineHandles[deviceIdx][j], &engineProp);
    if (result != ZE_RESULT_SUCCESS) {
       printf("Failed to get engine properties\n");
       exit(-1);
    }
    if (engineProp.type == ZES_ENGINE_GROUP_COMPUTE_ALL){
      zes_engine_stats_t engineStats = {0};
      result = zesEngineGetActivity(_sampling_engineHandles[deviceIdx][j], &engineStats);
      if (result != ZE_RESULT_SUCCESS) {
        printf("Failed to get engine activity data\n");
        exit(-1);
      }
      computeData[engineProp.subdeviceId].computeActive = engineStats.activeTime;
      computeData[engineProp.subdeviceId].timestamp = engineStats.timestamp;
    }
  }
}

void readCopyE(uint32_t deviceIdx, copyEngineData *copyData ){
  ze_result_t result;
  for (uint32_t i = 0; i < _sampling_subDeviceCount; i++) {
    copyData[i].copyActive = 0;
    copyData[i].timestamp = 0;
  }
  for (uint32_t j = 0; j < _sampling_engineCounts[deviceIdx]; ++j) {
    zes_engine_properties_t engineProp = {};
    result = zesEngineGetProperties(_sampling_engineHandles[deviceIdx][j], &engineProp);
    if (result != ZE_RESULT_SUCCESS) {
       printf("Failed to get engine properties\n");
       exit(-1);
    }
    if (engineProp.type == ZES_ENGINE_GROUP_COPY_ALL){
      zes_engine_stats_t engineStats = {0};
      result = zesEngineGetActivity(_sampling_engineHandles[deviceIdx][j], &engineStats);
      if (result != ZE_RESULT_SUCCESS) {
        printf("Failed to get engine activity data\n");
        exit(-1);
      }
      copyData[engineProp.subdeviceId].copyActive = engineStats.activeTime;
      copyData[engineProp.subdeviceId].timestamp = engineStats.timestamp;
    }
  }
}

static void thapi_sampling_energy() {
  uint64_t ts_us;
  uint64_t energy_uj;
  uint32_t frequency;
  computeEngineData computeE[_sampling_subDeviceCount];
  copyEngineData copyE[_sampling_subDeviceCount];
  for (uint32_t i = 0; i < _sampling_deviceCount; i++) {
    if (tracepoint_enabled(lttng_ust_ze_sampling, gpu_frequency)){
      for (uint32_t j = 0; j < _sampling_freqDomainCounts[i]; j++) {
        readFrequency(i, j, &frequency);
        do_tracepoint(lttng_ust_ze_sampling, gpu_frequency, (ze_device_handle_t)_sampling_hDevices[i], j, ts_us, frequency);
      }
    }
    if (tracepoint_enabled(lttng_ust_ze_sampling, gpu_energy)){
      for (uint32_t j = 0; j < _sampling_powerDomainCounts[i]; j++) {
        readEnergy(i, j, &ts_us, &energy_uj);
        do_tracepoint(lttng_ust_ze_sampling, gpu_energy, (ze_device_handle_t)_sampling_hDevices[i], j, (uint64_t)energy_uj, ts_us);
      }
    }
    if (tracepoint_enabled(lttng_ust_ze_sampling, computeEngine)){
      readComputeE(i, computeE);
      for (uint32_t k=0; k<_sampling_subDeviceCount; k++){
        do_tracepoint(lttng_ust_ze_sampling, computeEngine, (ze_device_handle_t)_sampling_hDevices[i], k, computeE[k].computeActive, computeE[k].timestamp);
      }
    }
    if (tracepoint_enabled(lttng_ust_ze_sampling, copyEngine)){
      readCopyE(i, copyE);
      for (uint32_t k=0; k<_sampling_subDeviceCount; k++){
        do_tracepoint(lttng_ust_ze_sampling, copyEngine, (ze_device_handle_t)_sampling_hDevices[i], k, copyE[k].copyActive, copyE[k].timestamp);
      }
    }
  }
}

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;
  struct timespec interval;
  thapi_sampling_init();

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

  if (getenv("LTTNG_UST_SAMPLING_ENERGY")) {
    initializeHandles();
    /* TODO: make it configurable */
    interval.tv_sec = 0;
    interval.tv_nsec = 50000000;
    thapi_sampling_energy();
    thapi_register_sampling(&thapi_sampling_energy, &interval);
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
