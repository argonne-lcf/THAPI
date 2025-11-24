#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdbool.h>

const char *find_lib_path(const char* lib_name, bool verbose) {
  void* handle = dlopen(lib_name, RTLD_LAZY);
  if (!handle) goto err0;

  struct link_map* lm = NULL;
  if (dlinfo(handle, RTLD_DI_LINKMAP, &lm) != 0) goto err1;

  char *lib_path = calloc(PATH_MAX + 1, sizeof(char));
  strncpy(lib_path, lm->l_name, PATH_MAX);

  dlclose(handle);

  return lib_path;

err1:
  dlclose(handle);
err0:
  if (verbose)
    fprintf(stderr, "libDlinfoWrapper.so: dlopen/dlinfo error: %s\n", dlerror());
  return NULL;
}
