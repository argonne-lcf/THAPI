static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile int _initialized = 0;

static void _load_tracer(void) {
  void * handle = dlopen("libcudart.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    fprintf(stderr, "Failure: could not load cudart library!\n");
    exit(1);
  }

  find_cudart_symbols(handle);
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

