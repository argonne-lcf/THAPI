static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile unsigned int _initialized = 0;

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  s = getenv("LTTNG_UST_MPI_LIBMPI");
  /* RTLD_GLOBAL: promote the real libmpi's symbols into the global scope so
   * that symbols we do NOT wrap (e.g. MPIX_GPU_query_support) remain resolvable
   * by the traced application. With RTLD_LOCAL they were hidden, causing
   * "undefined symbol" errors at lookup time. Our own preloaded wrappers still
   * take precedence for the symbols we do wrap (they are earlier in the global
   * scope), and RTLD_DEEPBIND keeps the real lib's internal calls bound to
   * itself rather than re-entering our wrappers. */
  if (s)
    handle = dlopen(s, RTLD_LAZY | RTLD_GLOBAL | RTLD_DEEPBIND);
  else
    handle = dlopen("libmpi.so", RTLD_LAZY | RTLD_GLOBAL | RTLD_DEEPBIND);
  if (handle) {
    void *ptr = dlsym(handle, "MPI_Init");
    if (ptr == (void *)&MPI_Init) { // opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if (!handle) {
    fprintf(stderr, "THAPI: Failure: could not load MPI library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_MPI_VERBOSE");
  if (s)
    verbose = 1;

  find_mpi_symbols(handle, verbose);
}

static inline void _init_tracer(void) {
  if (__builtin_expect(_initialized, 1))
    return;
  /* Avoid reentrancy */
  if (!in_init) {
    in_init = 1;
    __sync_synchronize();
    pthread_once(&_init, _load_tracer);
    __sync_synchronize();
    in_init = 0;
  }
  _initialized = 1;
}
