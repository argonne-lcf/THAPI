
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
  find_cuda_extensions();
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

