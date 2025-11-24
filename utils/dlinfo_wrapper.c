#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int find_lib_path(const char* lib_name, bool verbose) {
  void* handle = dlopen(lib_name, RTLD_LAZY);
  if (!handle) goto err0;

  struct link_map* lm = NULL;
  if (dlinfo(handle, RTLD_DI_LINKMAP, &lm) != 0) goto err1;
  fprintf(stdout, "%s", lm->l_name);

  dlclose(handle);

  return 0;
err1:
  dlclose(handle);
err0:
  if (verbose)
    fprintf(stderr, "[dlinfo_wrapper] dlopen/dlinfo error: %s\n", dlerror());
  return 1;
}

int main(int argc, char *argv[]) {
  // No error checking is done for input arguments. We expect the caller to
  // know what they are doing.
  return find_lib_path(argv[1], atoi(argv[2]) != 0);
}
