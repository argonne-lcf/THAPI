#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define check_error(ptr_)                                                      \
  {                                                                            \
    void *ptr = (void *)ptr_;                                                  \
    if (!ptr) {                                                                \
      fprintf(stderr, "%s:%d -- %s\n", __FILE__, __LINE__, dlerror());         \
      return 1;                                                                \
    }                                                                          \
  }

int main(void) {
  dlerror();

  for (int i = 0; i < 2; i++) {
    void *thapi = dlopen("libThapi.so", RTLD_NOW | RTLD_LOCAL);
    check_error(thapi);

    void (*start)(void) = (void (*)(void))dlsym(thapi, "thapi_start");
    check_error(start);

    void (*stop)(void) = (void (*)(void))dlsym(thapi, "thapi_stop");
    check_error(stop);

    (*start)(), (*stop)();

    dlclose(thapi);
  }

  return 0;
}

#undef check_error
