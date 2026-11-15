#include "tracer_init_utils.h"

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  handle = thapi_dlopen_real("LTTNG_UST_HIP_LIBAMDHIP64", "libamdhip64.so", RTLD_LAZY | RTLD_LOCAL,
                             "hipSetDevice", (void *)&hipSetDevice, "hip");

  s = getenv("LTTNG_UST_HIP_VERBOSE");
  if (s)
    verbose = 1;

  find_hip_symbols(handle, verbose);
}

THAPI_INIT_ONCE(_init_tracer, _load_tracer)
