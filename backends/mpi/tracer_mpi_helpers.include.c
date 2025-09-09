static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile unsigned int _initialized = 0;

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  s = getenv("LTTNG_UST_MPI_LIBMPI");
  if (s)
    handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  else
    handle = dlopen("libmpi.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  if (handle) {
    void* ptr = dlsym(handle, "MPI_Init");
    if (ptr == (void*)&MPI_Init) { //opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if( !handle ) {
    fprintf(stderr, "THAPI: Failure: could not load MPI library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_MPI_VERBOSE");
  if (s)
    verbose = 1;

  find_mpi_symbols(handle, verbose);
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

