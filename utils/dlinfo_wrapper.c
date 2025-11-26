#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "[dlinfo_wrapper] Usage: %s <lib_name> <verbosity>", argv[0]);
    return -1;
  }

  void* handle = dlopen(argv[1], RTLD_LAZY);
  int exit_code = !handle;
  if (exit_code) goto print_error_and_exit;

  struct link_map* lm = NULL;
  if ((exit_code = dlinfo(handle, RTLD_DI_LINKMAP, &lm)) == 0)
    fprintf(stdout, "%s", lm->l_name);

  exit_code |= dlclose(handle);

print_error_and_exit:
  if (exit_code && atoi(argv[2]))
    fprintf(stderr, "[dlinfo_wrapper] dlopen/dlinfo error: %s\n", dlerror());
  return exit_code;
}
