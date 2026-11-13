#include "tracer_init_utils.h"

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  handle = thapi_dlopen_real("LTTNG_UST_CUDART_LIBCUDART", "libcudart.so", RTLD_LAZY | RTLD_LOCAL,
                             "cudaSetDevice", dlsym(RTLD_DEFAULT, "cudaSetDevice"), "cudart");

  s = getenv("LTTNG_UST_CUDART_VERBOSE");
  if (s)
    verbose = 1;

  find_cudart_symbols(handle, verbose);
}

THAPI_INIT_ONCE(_init_tracer, _load_tracer)
