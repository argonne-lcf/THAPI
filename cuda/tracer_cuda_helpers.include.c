
static inline void _dump_kernel_args(CUfunction f, void **kernelParams, void** extra) {
  (void)extra;
  size_t argCount;
  CUresult res;
  int    count = 0;
  CUfunction_arg_desc_query q;

  if (tracepoint_enabled(lttng_ust_cuda_args, arg_count)) {
    res = CU_FUNCTION_GET_ARG_COUNT_PTR(f, &argCount);
    if (res != CUDA_SUCCESS)
      return;
    count = 1;
    do_tracepoint(lttng_ust_cuda_args, arg_count, f, argCount);
  }
  if (tracepoint_enabled(lttng_ust_cuda_args, arg_value)) {
    if (!count) {
      res = CU_FUNCTION_GET_ARG_COUNT_PTR(f, &argCount);
      if (res != CUDA_SUCCESS)
        return;
    }
    if (kernelParams) {
      for (size_t i = 0; i < argCount; i++) {
        q.sz = 0x28;
        res = CU_FUNCTION_GET_ARG_DESCRIPTOR_PTR(f, i, &q);
        if (res == CUDA_SUCCESS) {
          do_tracepoint(lttng_ust_cuda_args, arg_value, f, i, kernelParams[i], q.argSize);
        }
      }
    }
    if (extra) {
      void *argBuffer = NULL;
      size_t argBufferSize = 0;
      int indx = 0;
      while (extra[indx] != CU_LAUNCH_PARAM_END) {
        if ((void *)CU_LAUNCH_PARAM_BUFFER_POINTER == extra[indx]) {
          argBuffer = extra[indx+1];
        } else if ((void *)CU_LAUNCH_PARAM_BUFFER_SIZE == extra[indx]) {
          argBufferSize = *(size_t*)(extra + indx + 1);
        }
        indx += 2;
      }
      if (argBuffer && argBufferSize) {
        for (size_t i = 0; i < argCount; i++) {
          q.sz = 0x28;
          res = CU_FUNCTION_GET_ARG_DESCRIPTOR_PTR(f, i, &q);
          if (res == CUDA_SUCCESS && argBufferSize >= q.argOffset + q.argSize) {
            do_tracepoint(lttng_ust_cuda_args, arg_value, f, i, (void *)((intptr_t)argBuffer + q.argOffset), q.argSize);
          }
        }
      }
    }
  }
}

static int     _do_profile = 0;
static pthread_mutex_t _cuda_events_mutex = PTHREAD_MUTEX_INITIALIZER;

struct _cuda_event_s;
struct _cuda_event_s {
  struct _cuda_event_s *prev;
  struct _cuda_event_s *next;
  CUevent   start;
  CUevent   stop;
  CUcontext context;
};

struct _cuda_event_s * _events = NULL;

static inline void _register_cuda_event(CUevent hStart, CUevent hStop, CUcontext hContext) {
  struct _cuda_event_s *ev;

  ev = (struct _cuda_event_s *)calloc(sizeof(struct _cuda_event_s), 1);
  if (!ev)
    goto error;

  ev->start = hStart;
  ev->stop = hStop;
  ev->context = hContext;
  tracepoint(lttng_ust_cuda_profiling, event_profiling, hStart, hStop);
  pthread_mutex_lock(&_cuda_events_mutex);
  DL_APPEND(_events, ev);
  pthread_mutex_unlock(&_cuda_events_mutex);
  return;
error:
  CU_EVENT_DESTROY_V2_PTR(hStart);
  CU_EVENT_DESTROY_V2_PTR(hStop);
}

static inline CUevent _create_record_event(CUstream hStream) {
  CUevent hEvent;
  CUcontext streamContext;
  CUcontext currentContext;
  if (hStream) {
    if (CU_CTX_GET_CURRENT_PTR(&currentContext) != CUDA_SUCCESS)
      return NULL;
    if (CU_STREAM_GET_CTX_PTR(hStream, &streamContext) != CUDA_SUCCESS)
      return NULL;
    if (streamContext != currentContext) {
      if (CU_CTX_PUSH_CURRENT_PTR(streamContext) != CUDA_SUCCESS)
        return NULL;
    }
  }
  if(CU_EVENT_CREATE_PTR(&hEvent, CU_EVENT_DEFAULT) != CUDA_SUCCESS) {
    hEvent = NULL;
  } else {
    if(CU_EVENT_RECORD_PTR(hEvent, hStream) != CUDA_SUCCESS) {
      CU_EVENT_DESTROY_V2_PTR(hEvent);
      hEvent = NULL;
    }
  }
  if (hStream && streamContext != currentContext) {
    CU_CTX_POP_CURRENT_PTR(&streamContext);
  }
  return hEvent;
}

static inline void _event_profile(CUresult status, CUevent hStart, CUstream hStream) {
  CUevent hStop;
  CUcontext hContext;
  if (status != CUDA_SUCCESS) {
    CU_EVENT_DESTROY_V2_PTR(hStart);
    return;
  }
  if (hStart) {
    hStop = _create_record_event(hStream);
    if (!hStop) {
      CU_EVENT_DESTROY_V2_PTR(hStart);
      return;
    }
    if (hStream)
      CU_STREAM_GET_CTX_PTR(hStream, &hContext);
    else
      CU_CTX_GET_CURRENT_PTR(&hContext);
    _register_cuda_event(hStart, hStop, hContext);
  }
}

static void _profile_event_results(struct _cuda_event_s *ev) {
  float milliseconds;
  CUresult startStatus, stopStatus, status;

  if (tracepoint_enabled(lttng_ust_cuda_profiling, event_profiling_results)) {
    startStatus = CU_EVENT_QUERY_PTR(ev->start);
    stopStatus = CU_EVENT_QUERY_PTR(ev->stop);
    status = CU_EVENT_ELAPSED_TIME_PTR(&milliseconds, ev->start, ev->stop);
    do_tracepoint(lttng_ust_cuda_profiling, event_profiling_results,
                  ev->start, ev->stop, startStatus, stopStatus,
                  status, milliseconds);
  }
}

static void _event_cleanup() {
  struct _cuda_event_s *ev, *tmp;

  DL_FOREACH_SAFE(_events, ev, tmp) {
    DL_DELETE(_events, ev);
    _profile_event_results(ev);
    CU_EVENT_DESTROY_V2_PTR(ev->start);
    CU_EVENT_DESTROY_V2_PTR(ev->stop);
    free(ev);
  }
}

static void _lib_cleanup() {
  if (_do_profile) {
    _event_cleanup();
  }
}

static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile int _initialized = 0;

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;

  s = getenv("LTTNG_UST_CUDA_LIBCUDA");
  if (s)
      handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL);
  else
      handle = dlopen("libcuda.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    fprintf(stderr, "Failure: could not load cuda library!\n");
    exit(1);
  }

  find_cuda_symbols(handle);
  CU_INIT_PTR(0);
  find_cuda_extensions();

  s = getenv("LTTNG_UST_CUDA_PROFILE");
  if (s)
    _do_profile = 1;

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

