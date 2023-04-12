static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile int _initialized = 0;

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  s = getenv("LTTNG_UST_HIP_LIBHIP");
  if (s)
      handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL);
  else
      handle = dlopen("libamdhip64.so", RTLD_LAZY | RTLD_LOCAL);
  if (handle) {
    void* ptr = dlsym(handle, "hipSetDevice");
    if (ptr == (void*)&hipSetDevice) { //opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if( !handle ) {
    fprintf(stderr, "Failure: could not load hip library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_HIP_VERBOSE");
  if (s)
    verbose = 1;

  find_hip_symbols(handle, verbose);
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

