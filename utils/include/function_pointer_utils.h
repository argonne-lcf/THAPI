#pragma once
/* A function the application gets as a pointer (from an extension-function
 * lookup) cannot be interposed. The tracer hands out a libffi closure instead:
 * calling it calls the tracer's wrapper, which receives the real function. */
#include "uthash.h"
#include <ffi.h>
#include <pthread.h>
#include <stdlib.h>

/* wrapper(cif, where the result goes, the arguments, the real function) */
typedef void (*thapi_closure_wrapper)(ffi_cif *, void *, void **, void *);

struct thapi_closure {
  void *ptr;   /* the real function: the hash key */
  void *c_ptr; /* what the application calls */
  UT_hash_handle hh;
  ffi_cif cif;
  ffi_closure *closure;
};

static struct thapi_closure *thapi_closures = NULL;
static pthread_mutex_t thapi_closures_mutex = PTHREAD_MUTEX_INITIALIZER;

/* What to hand out in place of `real`, or NULL when libffi fails. Each driver
 * returns its own function, so there is one closure per real pointer, made the
 * first time it is asked for. `types` must outlive the closure. */
static inline void *thapi_closure_for(void *real,
                                      thapi_closure_wrapper wrapper,
                                      ffi_type *rtype,
                                      unsigned int nargs,
                                      ffi_type **types) {
  struct thapi_closure *closure = NULL;
  pthread_mutex_lock(&thapi_closures_mutex);
  HASH_FIND_PTR(thapi_closures, &real, closure);
  if (closure == NULL &&
      (closure = (struct thapi_closure *)malloc(sizeof(struct thapi_closure))) != NULL) {
    closure->ptr = real;
    closure->closure = (ffi_closure *)ffi_closure_alloc(sizeof(ffi_closure), &(closure->c_ptr));
    if (closure->closure != NULL &&
        ffi_prep_cif(&(closure->cif), FFI_DEFAULT_ABI, nargs, rtype, types) == FFI_OK &&
        ffi_prep_closure_loc(closure->closure, &(closure->cif), wrapper, real, closure->c_ptr) ==
            FFI_OK) {
      HASH_ADD_PTR(thapi_closures, ptr, closure);
    } else {
      if (closure->closure != NULL)
        ffi_closure_free(closure->closure);
      free(closure);
      closure = NULL;
    }
  }
  pthread_mutex_unlock(&thapi_closures_mutex);
  return closure ? closure->c_ptr : NULL;
}
