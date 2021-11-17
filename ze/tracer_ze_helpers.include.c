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

struct _ze_command_list_obj_data {
  ze_device_handle_t device;
  ze_context_handle_t context;
  ze_driver_handle_t driver;
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

#define FIND_ZE_OBJ(key, val) { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_FIND_PTR(_ze_objs, key, val); \
  pthread_mutex_unlock(&_ze_objs_mutex); \
}

#define ADD_ZE_OBJ(val) { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_ADD_PTR(_ze_objs, ptr, val); \
  pthread_mutex_unlock(&_ze_objs_mutex); \
}

#define FIND_AND_DEL_ZE_OBJ(key, val) { \
  pthread_mutex_lock(&_ze_objs_mutex); \
  HASH_FIND_PTR(_ze_objs, key, val); \
  if (val) { \
    HASH_DEL(_ze_objs, val); \
  } \
  pthread_mutex_unlock(&_ze_objs_mutex); \
}

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
  ze_result_t res = zeDeviceGetProperties(device, &(d_data->properties));
  if (res != ZE_RESULT_SUCCESS) {
    free((void *)mem);
    return;
  }

  ADD_ZE_OBJ(o_h);
}

static inline void _register_ze_command_list(
 ze_command_list_handle_t command_list,
 ze_context_handle_t context,
 ze_device_handle_t device) {
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
  o_h->obj_data = (void *)cl_data;

  ADD_ZE_OBJ(o_h);
}

struct _ze_event_h {
  ze_event_handle_t event;
  UT_hash_handle hh;
  ze_command_list_handle_t command_list;
  ze_event_pool_handle_t event_pool;
  ze_context_handle_t context;
};

#define FIND_ZE_EVENT(key, val) { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_FIND_PTR(_ze_events, key, val); \
  pthread_mutex_unlock(&_ze_events_mutex); \
}

#define ADD_ZE_EVENT(val) { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_ADD_PTR(_ze_events, event, val); \
  pthread_mutex_unlock(&_ze_events_mutex); \
}

#define FIND_AND_DEL_ZE_EVENT(key, val) { \
  pthread_mutex_lock(&_ze_events_mutex); \
  HASH_FIND_PTR(_ze_events, key, val); \
  if (val) { \
    HASH_DEL(_ze_events, val); \
  } \
  pthread_mutex_unlock(&_ze_events_mutex); \
}

static struct _ze_event_h *_ze_events = NULL;
static pthread_mutex_t _ze_events_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void _register_ze_event(
 ze_event_handle_t event,
 ze_command_list_handle_t command_list,
 ze_event_pool_handle_t event_pool) {
  struct _ze_event_h *_ze_event = NULL;
  ze_context_handle_t context = NULL;

  FIND_ZE_EVENT(&event, _ze_event);
  if (_ze_event)
    return;

  struct _ze_obj_h *o_h = NULL;
  FIND_ZE_OBJ(&command_list, o_h);
  if (o_h)
    context = ((struct _ze_command_list_obj_data *)(o_h->obj_data))->context;

  _ze_event = (struct _ze_event_h *)calloc(1, sizeof(struct _ze_event_h));
  if (!_ze_event)
    return;

  _ze_event->event = event;
  _ze_event->command_list = command_list;
  _ze_event->event_pool = event_pool;
  _ze_event->context = context;

  ADD_ZE_EVENT(_ze_event);
}

static ze_event_handle_t _get_profiling_event(
 ze_command_list_handle_t command_list,
 ze_event_pool_handle_t *pool_ret) {
  struct _ze_obj_h *o_h = NULL;

  FIND_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return NULL;
  ze_context_handle_t context =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->context;
  ze_device_handle_t device =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->device;

  ze_event_pool_handle_t event_pool = NULL;
  ze_event_pool_desc_t desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, NULL, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(context, &desc, 1, &device, &event_pool);
  if (res != ZE_RESULT_SUCCESS)
    return NULL;
  ze_event_handle_t event;
  ze_event_desc_t e_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, NULL, 0, 0, 0};
  res = ZE_EVENT_CREATE_PTR(event_pool, &e_desc, &event);
  if (res != ZE_RESULT_SUCCESS) {
    ZE_EVENT_POOL_DESTROY_PTR(event_pool);
    return NULL;
  }
  *pool_ret = event_pool;
  return event;
}

static void _profile_event_results(ze_event_handle_t event);

static inline void _unregister_ze_event(ze_event_handle_t event) {
  struct _ze_event_h *ze_event = NULL;

  FIND_AND_DEL_ZE_EVENT(&event, ze_event);
  if (!ze_event)
    return;

  _profile_event_results(event);
  free(ze_event);
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
    if (ze_event->event)
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
      if (ze_event->event)
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
}

static void _unregister_ze_command_list(ze_command_list_handle_t command_list) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return;

  free(o_h);
}

static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile unsigned int _initialized = 0;
static int _do_profile = 0;
static int _paranoid_drift = 0;

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
  for (uint32_t i = 0; i < driverCount; i++) {
    _dump_driver_device_properties(phDrivers[i]);
  }

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

  find_ze_symbols(handle, verbose);

  //FIX for intel tracing layer that needs to register its callbacks first...
  ZE_INIT_PTR(0);

  if (tracepoint_enabled(lttng_ust_ze_properties, driver) || tracepoint_enabled(lttng_ust_ze_properties, device) || tracepoint_enabled(lttng_ust_ze_properties, subdevice))
    _dump_properties();
  s = getenv("LTTNG_UST_ZE_PROFILE");
  if (s)
    _do_profile = 1;
  s = getenv("LTTNG_UST_ZE_PARANOID_DRIFT");
  if (s)
    _paranoid_drift = 1;
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
