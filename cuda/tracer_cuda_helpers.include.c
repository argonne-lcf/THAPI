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

