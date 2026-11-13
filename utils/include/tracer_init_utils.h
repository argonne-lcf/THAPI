#pragma once
/* What an interposing tracer does once, the first time one of its functions is
 * called: open the library it stands in front of. */
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* The real library: the one the `env` variable names, else `name`, opened with
 * `flags`. The tracer is preloaded under the same name, so dlopen can hand the
 * tracer back: that is the case when `symbol` resolves to `self`, the
 * tracer's own definition of it. Without a real library there is nothing to
 * forward to: exit. */
static inline void *thapi_dlopen_real(const char *env,
                                      const char *name,
                                      int flags,
                                      const char *symbol,
                                      void *self,
                                      const char *what) {
  const char *path = getenv(env);
  void *handle = dlopen(path ? path : name, flags);
  if (handle && dlsym(handle, symbol) == self) {
    dlclose(handle);
    handle = NULL;
  }
  if (!handle) {
    fprintf(stderr, "THAPI: Failure: could not load %s library!\n", what);
    exit(1);
  }
  return handle;
}

/* Define `name()`, which runs `load` the first time it is called. `load` calls
 * into the real library, which can call a traced function back on the same
 * thread: that call returns at once instead of waiting on itself. */
#define THAPI_INIT_ONCE(name, load)                                                                \
  static pthread_once_t name##_once = PTHREAD_ONCE_INIT;                                           \
  static __thread volatile int name##_running = 0;                                                 \
  static volatile int name##_done = 0;                                                             \
  static inline void name(void) {                                                                  \
    if (__builtin_expect(name##_done, 1))                                                          \
      return;                                                                                      \
    if (!name##_running) {                                                                         \
      name##_running = 1;                                                                          \
      __sync_synchronize();                                                                        \
      pthread_once(&name##_once, load);                                                            \
      __sync_synchronize();                                                                        \
      name##_running = 0;                                                                          \
    }                                                                                              \
    name##_done = 1;                                                                               \
  }
