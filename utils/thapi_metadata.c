#include "thapi_metadata_tracepoints.h"
#include <stdlib.h>

int main() {
  char *s = NULL;
  s = getenv("LTTNG_UST_THAPI_METADATA");
  if (s)
    tracepoint(lttng_ust_thapi, metadata, s);
  return 0;
}
