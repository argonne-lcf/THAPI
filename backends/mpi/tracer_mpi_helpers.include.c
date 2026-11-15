#include "tracer_init_utils.h"

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  /* RTLD_GLOBAL: promote the real libmpi's symbols into the global scope so
   * that symbols we do NOT wrap (e.g. MPIX_GPU_query_support) remain resolvable
   * by the traced application. With RTLD_LOCAL they were hidden, causing
   * "undefined symbol" errors at lookup time. Our own preloaded wrappers still
   * take precedence for the symbols we do wrap (they are earlier in the global
   * scope), and RTLD_DEEPBIND keeps the real lib's internal calls bound to
   * itself rather than re-entering our wrappers. */
  handle = thapi_dlopen_real("LTTNG_UST_MPI_LIBMPI", "libmpi.so",
                             RTLD_LAZY | RTLD_GLOBAL | RTLD_DEEPBIND, "MPI_Init", (void *)&MPI_Init,
                             "MPI");

  s = getenv("LTTNG_UST_MPI_VERBOSE");
  if (s)
    verbose = 1;

  find_mpi_symbols(handle, verbose);
}

THAPI_INIT_ONCE(_init_tracer, _load_tracer)
