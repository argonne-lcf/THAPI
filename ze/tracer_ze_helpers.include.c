enum _ze_obj_type {
  UNKNOWN = 0,
  DRIVER,
  DEVICE,
  COMMAND_LIST
};

struct _ze_device_obj_data {
  ze_driver_handle_t driver;
  ze_device_properties_t properties;
};

struct _ze_command_list_obj_data {
  ze_device_handle_t device;
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
 ze_driver_handle_t driver) {
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
  o_h->obj_data = (void *)d_data;

  d_data->properties.version = ZE_DEVICE_PROPERTIES_VERSION_CURRENT;
  ze_result_t res = zeDeviceGetProperties(device, &(d_data->properties));
  if (res != ZE_RESULT_SUCCESS) {
    free((void *)mem);
    return;
  }

  ADD_ZE_OBJ(o_h);
}

static inline void _register_ze_command_list(
 ze_command_list_handle_t command_list,
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
  cl_data->driver = driver;
  o_h->obj_data = (void *)cl_data;

  ADD_ZE_OBJ(o_h);
}

static inline void _unregister_ze_command_list(
 ze_command_list_handle_t command_list ) {
  struct _ze_obj_h *o_h = NULL;

  FIND_AND_DEL_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return;
  free(o_h);
}

struct _ze_event_h {
  ze_event_handle_t event;
  UT_hash_handle hh;
  ze_command_list_handle_t command_list;
  ze_event_pool_handle_t event_pool;
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

struct _ze_event_h *_ze_events = NULL;
pthread_mutex_t _ze_events_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void _register_ze_event(
 ze_event_handle_t event,
 ze_command_list_handle_t command_list,
 ze_event_pool_handle_t event_pool) {
  struct _ze_event_h *_ze_event = NULL;

  FIND_ZE_EVENT(&event, _ze_event);
  if (_ze_event)
    return;

  _ze_event = (struct _ze_event_h *)calloc(1, sizeof(struct _ze_event_h));
  if (!_ze_event)
    return;

  _ze_event->event = event;
  _ze_event->command_list = command_list;
  _ze_event->event_pool = event_pool;

  ADD_ZE_EVENT(_ze_event);
}

ze_event_handle_t _get_profiling_event(
 ze_command_list_handle_t command_list,
 ze_event_pool_handle_t *pool_ret) {
  struct _ze_obj_h *o_h = NULL;

  FIND_ZE_OBJ(&command_list, o_h);
  if (!o_h)
    return NULL;
  ze_driver_handle_t driver =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->driver;
  ze_device_handle_t device =
    ((struct _ze_command_list_obj_data *)(o_h->obj_data))->device;

  ze_event_pool_handle_t event_pool = NULL;
  ze_event_pool_desc_t desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT, ZE_EVENT_POOL_FLAG_TIMESTAMP, 1};
  ze_result_t res = ZE_EVENT_POOL_CREATE_PTR(driver, &desc, 1, &device, &event_pool);
  if (res != ZE_RESULT_SUCCESS)
    return NULL;
  ze_event_handle_t event;
  ze_event_desc_t e_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
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
  ze_result_t status;
  ze_result_t global_start_status;
  uint64_t global_start;
  ze_result_t global_end_status;
  uint64_t global_end;
  ze_result_t context_start_status;
  uint64_t context_start;
  ze_result_t context_end_status;
  uint64_t context_end;

  if (tracepoint_enabled(lttng_ust_ze_profiling, event_profiling_results)) {
    status = ZE_EVENT_QUERY_STATUS_PTR(event);
    global_start_status = ZE_EVENT_GET_TIMESTAMP_PTR(
      event,
      ZE_EVENT_TIMESTAMP_GLOBAL_START,
      &global_start);
    global_end_status = ZE_EVENT_GET_TIMESTAMP_PTR(
      event,
      ZE_EVENT_TIMESTAMP_GLOBAL_END,
      &global_end);
    context_start_status = ZE_EVENT_GET_TIMESTAMP_PTR(
      event,
      ZE_EVENT_TIMESTAMP_CONTEXT_START,
      &context_start);
    context_end_status = ZE_EVENT_GET_TIMESTAMP_PTR(
      event,
      ZE_EVENT_TIMESTAMP_CONTEXT_END,
      &context_end);
    do_tracepoint(lttng_ust_ze_profiling, event_profiling_results,
                  event, status,
                  global_start_status, global_start,
                  global_end_status, global_end,
                  context_start_status, context_start,
                  context_end_status, context_end);
  }
}

void _event_cleanup() {
  struct _ze_event_h *ze_event = NULL;
  struct _ze_event_h *tmp = NULL;

  HASH_ITER(hh, _ze_events, ze_event, tmp) {
    HASH_DEL(_ze_events, ze_event);

    if (ze_event->event) {
      _profile_event_results(ze_event->event);
      ZE_EVENT_DESTROY_PTR(ze_event->event);
    }
    if (ze_event->event_pool) {
      ZE_EVENT_POOL_DESTROY_PTR(ze_event->event_pool);
    }
    free(ze_event);
  }
}

static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile cl_uint _initialized = 0;
static int     _do_profile = 0;

void _lib_cleanup() {
  if (_do_profile) {
    _event_cleanup();
  }
}

static void _load_tracer(void) {
  void * handle = dlopen("libze_loader.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    fprintf(stderr, "Failure: could not load ze library!\n");
    exit(1);
  }

  char *s = NULL;
  s = getenv("LTTNG_UST_ZE_PROFILE");
  if (s)
    _do_profile = 1;

  if (_do_profile)
    atexit(&_lib_cleanup);

  find_ze_symbols(handle);
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

