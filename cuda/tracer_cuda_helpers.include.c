#include "thapi_sampling.h"

//pthread_mutex_t cuda_closures_mutex = PTHREAD_MUTEX_INITIALIZER;
//
//struct cuda_closure {
//  void *ptr;
//  void *c_ptr;
//  UT_hash_handle hh;
//  ffi_cif cif;
//  ffi_closure *closure;
//  ffi_type **types;
//};
//
//struct cuda_closure * cuda_closures = NULL;
//

#ifdef THAPI_USE_DESTRUCTORS
#define THAPI_ATTRIBUTE_DESTRUCTOR __attribute__((destructor))
#else
#define THAPI_ATTRIBUTE_DESTRUCTOR
#endif

static void _log_export(CUuuid *pExportTableId, size_t exportOffset) {
  tracepoint(lttng_ust_cuda_exports, export_called, pExportTableId, exportOffset);
}

#define WRAPPER_SIZE 0x50
union _ptr_u {
  intptr_t ptr;
  unsigned char s[8];
};

static void _wrap_export(void *func, CUuuid *pExportTableId, size_t offset,
                         void **pDestTable, void *pDest) {
  union _ptr_u f = {.ptr = (intptr_t)func };
  union _ptr_u u = {.ptr = (intptr_t)pExportTableId };
  union _ptr_u o = {.ptr = (intptr_t)offset };
  union _ptr_u l = {.ptr = (intptr_t)&_log_export};
/*
   0:	57                   	push   %rdi
   1:	56                   	push   %rsi
   2:	52                   	push   %rdx
   3:	51                   	push   %rcx
   4:	41 50                	push   %r8
   6:	41 51                	push   %r9
   8:	41 52                	push   %r10
   a:	41 53                	push   %r11
   c:	48 b8 f0 ee db ea 0d 	movabs $0xdeadbeef0,%rax
  13:	00 00 00
  16:	48 bf f1 ee db ea 0d 	movabs $0xdeadbeef1,%rdi
  1d:	00 00 00
  20:	48 be f2 ee db ea 0d 	movabs $0xdeadbeef2,%rsi
  27:	00 00 00
  2a:	ff d0                	callq  *%rax
  2c:	41 5b                	pop    %r11
  2e:	41 5a                	pop    %r10
  30:	41 59                	pop    %r9
  32:	41 58                	pop    %r8
  34:	59                   	pop    %rcx
  35:	5a                   	pop    %rdx
  36:	5e                   	pop    %rsi
  37:	5f                   	pop    %rdi
  38:	48 b8 f4 ee db ea 0d 	movabs $0xdeadbeef4,%rax
  3f:	00 00 00
  42:	ff e0                	jmpq   *%rax
*/

  unsigned char code[] = {
    /* Saving registers */
    0x57,
    0x56,
    0x52,
    0x51,
    0x41, 0x50,
    0x41, 0x51,
    0x41, 0x52,
    0x41, 0x53,
    /* Calling _log_export */
    0x48, 0xb8,
    l.s[0], l.s[1], l.s[2], l.s[3], l.s[4], l.s[5], l.s[6], l.s[7],
    0x48, 0xbf,
    u.s[0], u.s[1], u.s[2], u.s[3], u.s[4], u.s[5], u.s[6], u.s[7],
    0x48, 0xbe,
    o.s[0], o.s[1], o.s[2], o.s[3], o.s[4], o.s[5], o.s[6], o.s[7],
    0xff, 0xd0,
    /* Restoring registers */
    0x41, 0x5b,
    0x41, 0x5a,
    0x41, 0x59,
    0x41, 0x58,
    0x59,
    0x5a,
    0x5e,
    0x5f,
    /* Call original export */
    0x48, 0xb8,
    f.s[0], f.s[1], f.s[2], f.s[3], f.s[4], f.s[5], f.s[6], f.s[7],
    0xff, 0xe0 };

  memcpy(pDest, code, sizeof(code));
  *pDestTable = pDest;
}

static const void * _wrap_export_table(const void *pExportTable, const CUuuid *pExportTableId) {
  size_t export_table_sz = *(size_t*)pExportTable;
  size_t num_entries = (export_table_sz - sizeof(size_t))/sizeof(void*);
  size_t sz = WRAPPER_SIZE * num_entries + export_table_sz + sizeof(CUuuid);

  void *mem = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED)
    return pExportTable;

  char *puuid = (char *)mem + WRAPPER_SIZE * num_entries + export_table_sz;
  void **entries = (void **)((intptr_t)pExportTable + sizeof(size_t));
  size_t *newExportTable = (size_t *)((intptr_t)mem + WRAPPER_SIZE * num_entries);
  void **new_entries = (void **)((intptr_t)newExportTable + sizeof(size_t));

  *newExportTable = export_table_sz;
  memcpy(puuid, pExportTableId, sizeof(CUuuid));

  for(size_t i = 0; i < num_entries; i++) {
    if (entries[i]) {
      void * wrap;
      if ((wrap = cuda_extension_dispatcher(pExportTableId, sizeof(size_t) + i * sizeof(void*))))
        new_entries[i] = wrap;
      else
        _wrap_export(entries[i], (void *)puuid,
                     sizeof(size_t) + i * sizeof(void*),
                     new_entries + i,
                     (void**)((intptr_t)mem + i * WRAPPER_SIZE));
    } else
      new_entries[i] = entries[i];
  }

  if (mprotect(mem, sz, PROT_READ|PROT_EXEC)) {
    munmap(mem, sz);
    return pExportTable;
  }
  return (void*)((intptr_t)mem + WRAPPER_SIZE * num_entries);
}

static const void * _wrap_buggy_export_table(const void *pExportTable, const CUuuid *pExportTableId) {
  size_t export_table_sz = 0;
  while(*(void **)((intptr_t)pExportTable + export_table_sz))
    export_table_sz += 8;

  size_t num_entries = (export_table_sz)/sizeof(void*);
  size_t sz = WRAPPER_SIZE * num_entries + export_table_sz + sizeof(CUuuid);

  void *mem = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED)
    return pExportTable;

  char *puuid = (char *)mem + WRAPPER_SIZE * num_entries + export_table_sz;
  void **entries = (void **)pExportTable;
  size_t *newExportTable = (size_t *)((intptr_t)mem + WRAPPER_SIZE * num_entries);
  void **new_entries = (void **)newExportTable;

  memcpy(puuid, pExportTableId, sizeof(CUuuid));

  for(size_t i = 0; i < num_entries; i++) {
    void * wrap;
    if ((wrap = cuda_extension_dispatcher(pExportTableId, i * sizeof(void*))))
      new_entries[i] = wrap;
    else
      _wrap_export(entries[i], (void *)puuid,
                   i * sizeof(void*),
                   new_entries + i,
                   (void**)((intptr_t)mem + i * WRAPPER_SIZE));
  }

  if (mprotect(mem, sz, PROT_READ|PROT_EXEC)) {
    munmap(mem, sz);
    return pExportTable;
  }
  return (void*)((intptr_t)mem + WRAPPER_SIZE * num_entries);
}

static int _do_trace_export_tables = 0;
static pthread_mutex_t _cuda_export_tables_mutex = PTHREAD_MUTEX_INITIALIZER;

struct _export_table_h {
  CUuuid uuid;
  const void * export_table;
  UT_hash_handle hh;
};

static struct _export_table_h *_export_tables = NULL;

static const void * _wrap_and_cache_export_table(const void *pExportTable, const CUuuid *pExportTableId) {
  if (!pExportTable)
    return NULL;
  struct _export_table_h *export_table_h = NULL;
  pthread_mutex_lock(&_cuda_export_tables_mutex);
  HASH_FIND(hh, _export_tables, pExportTableId, sizeof(CUuuid), export_table_h);
  if (export_table_h) {
    pthread_mutex_unlock(&_cuda_export_tables_mutex);
    return export_table_h->export_table;
  }
  export_table_h = calloc(sizeof(struct _export_table_h), 1);
  if (!export_table_h) {
    pthread_mutex_unlock(&_cuda_export_tables_mutex);
    return pExportTable;
  }
  export_table_h->uuid = *pExportTableId;
  if (*(size_t*)pExportTable < 0x1000)
    export_table_h->export_table = _wrap_export_table(pExportTable, pExportTableId);
  else
    export_table_h->export_table = _wrap_buggy_export_table(pExportTable, pExportTableId);
  HASH_ADD(hh, _export_tables, uuid, sizeof(CUuuid), export_table_h);
  pthread_mutex_unlock(&_cuda_export_tables_mutex);
  return export_table_h->export_table;
}

static inline void _dump_device_properties() {
  int count = 0;
  CUresult res;
  res = CU_DEVICE_GET_COUNT_PTR(&count);
  if (res != CUDA_SUCCESS)
    return;
  for (int ordinal = 0; ordinal < count; ordinal++) {
    CUdevice device;
    CUuuid   uuid;
    char name[256];
    res = CU_DEVICE_GET_PTR(&device, ordinal);
    if (res != CUDA_SUCCESS)
      continue;
    res = CU_DEVICE_GET_NAME_PTR(name, 256, device);
    if (res != CUDA_SUCCESS)
      continue;
    res = CU_DEVICE_GET_UUID_PTR(&uuid, device);
    if (res != CUDA_SUCCESS)
      continue;
    do_tracepoint(lttng_ust_cuda_properties, device, ordinal, device, name, &uuid);
  }
}

static void _dump_properties() {
  if (tracepoint_enabled(lttng_ust_cuda_properties, device)) {
    _dump_device_properties();
  }
}

static inline void _dump_kernel_args(CUfunction f, void **kernelParams, void** extra) {
  (void)extra;
  size_t argCount;
  CUresult res;
  int    count = 0;
  cuexp_function_arg_desc_query q;

  if (tracepoint_enabled(lttng_ust_cuda_args, arg_count)) {
    res = CUEXP_FUNCTION_GET_ARG_COUNT_PTR(f, &argCount);
    if (res != CUDA_SUCCESS)
      return;
    count = 1;
    do_tracepoint(lttng_ust_cuda_args, arg_count, f, argCount);
  }
  if (tracepoint_enabled(lttng_ust_cuda_args, arg_value)) {
    if (!count) {
      res = CUEXP_FUNCTION_GET_ARG_COUNT_PTR(f, &argCount);
      if (res != CUDA_SUCCESS)
        return;
    }
    if (kernelParams) {
      for (size_t i = 0; i < argCount; i++) {
        q.sz = 0x28;
        res = CUEXP_FUNCTION_GET_ARG_DESCRIPTOR_PTR(f, i, &q);
        if (res == CUDA_SUCCESS) {
          do_tracepoint(lttng_ust_cuda_args, arg_value, f, i, kernelParams[i], q.argSize);
        }
      }
    }
    if (extra) {
      void *argBuffer = NULL;
      size_t argBufferSize = 0;
      int indx = 0;
      while (extra[indx] != CU_LAUNCH_PARAM_END) {
        if ((void *)CU_LAUNCH_PARAM_BUFFER_POINTER == extra[indx]) {
          argBuffer = extra[indx+1];
        } else if ((void *)CU_LAUNCH_PARAM_BUFFER_SIZE == extra[indx]) {
          argBufferSize = *(size_t*)(extra + indx + 1);
        }
        indx += 2;
      }
      if (argBuffer && argBufferSize) {
        for (size_t i = 0; i < argCount; i++) {
          q.sz = 0x28;
          res = CUEXP_FUNCTION_GET_ARG_DESCRIPTOR_PTR(f, i, &q);
          if (res == CUDA_SUCCESS && argBufferSize >= q.argOffset + q.argSize) {
            do_tracepoint(lttng_ust_cuda_args, arg_value, f, i, (void *)((intptr_t)argBuffer + q.argOffset), q.argSize);
          }
        }
      }
    }
  }
}

static int _do_profile = 0;
static int _do_cleanup = 0;
static pthread_mutex_t _cuda_events_mutex = PTHREAD_MUTEX_INITIALIZER;

struct _cuda_event_s;
struct _cuda_event_s {
  struct _cuda_event_s *prev;
  struct _cuda_event_s *next;
  CUevent   start;
  CUevent   stop;
  CUcontext context;
};

struct _cuda_event_s * _events = NULL;

static inline void _register_cuda_event(CUevent hStart, CUevent hStop, CUcontext hContext) {
  struct _cuda_event_s *ev;

  ev = (struct _cuda_event_s *)calloc(sizeof(struct _cuda_event_s), 1);
  if (!ev)
    goto error;

  ev->start = hStart;
  ev->stop = hStop;
  ev->context = hContext;
  tracepoint(lttng_ust_cuda_profiling, event_profiling, hStart, hStop);
  pthread_mutex_lock(&_cuda_events_mutex);
  DL_APPEND(_events, ev);
  pthread_mutex_unlock(&_cuda_events_mutex);
  return;
error:
  CU_EVENT_DESTROY_V2_PTR(hStart);
  CU_EVENT_DESTROY_V2_PTR(hStop);
}

static inline CUevent _create_record_event(CUstream hStream) {
  CUevent hEvent;
  CUcontext streamContext;
  CUcontext currentContext;
  if (hStream) {
    if (CU_CTX_GET_CURRENT_PTR(&currentContext) != CUDA_SUCCESS)
      return NULL;
    if (CU_STREAM_GET_CTX_PTR(hStream, &streamContext) != CUDA_SUCCESS)
      return NULL;
    if (streamContext != currentContext) {
      if (CU_CTX_PUSH_CURRENT_PTR(streamContext) != CUDA_SUCCESS)
        return NULL;
    }
  }
  if(CU_EVENT_CREATE_PTR(&hEvent, CU_EVENT_DEFAULT) != CUDA_SUCCESS) {
    hEvent = NULL;
  } else {
    if(CU_EVENT_RECORD_PTR(hEvent, hStream) != CUDA_SUCCESS) {
      CU_EVENT_DESTROY_V2_PTR(hEvent);
      hEvent = NULL;
    }
  }
  if (hStream && streamContext != currentContext) {
    CU_CTX_POP_CURRENT_PTR(&streamContext);
  }
  return hEvent;
}

static inline void _event_profile(CUresult status, CUevent hStart, CUstream hStream) {
  CUevent hStop;
  CUcontext hContext;
  if (status != CUDA_SUCCESS) {
    CU_EVENT_DESTROY_V2_PTR(hStart);
    return;
  }
  if (hStart) {
    hStop = _create_record_event(hStream);
    if (!hStop) {
      CU_EVENT_DESTROY_V2_PTR(hStart);
      return;
    }
    if (hStream)
      CU_STREAM_GET_CTX_PTR(hStream, &hContext);
    else
      CU_CTX_GET_CURRENT_PTR(&hContext);
    _register_cuda_event(hStart, hStop, hContext);
  }
}

static void _profile_event_results(struct _cuda_event_s *ev) {
  float milliseconds;
  CUresult startStatus, stopStatus, status;

  if (tracepoint_enabled(lttng_ust_cuda_profiling, event_profiling_results)) {
    startStatus = CU_EVENT_QUERY_PTR(ev->start);
    stopStatus = CU_EVENT_QUERY_PTR(ev->stop);
    status = CU_EVENT_ELAPSED_TIME_PTR(&milliseconds, ev->start, ev->stop);
    do_tracepoint(lttng_ust_cuda_profiling, event_profiling_results,
                  ev->start, ev->stop, startStatus, stopStatus,
                  status, milliseconds);
  }
}

static void _context_event_cleanup(CUcontext hContext) {
  struct _cuda_event_s *ev, *tmp;
  struct _cuda_event_s *evs = NULL;

  pthread_mutex_lock(&_cuda_events_mutex);
  DL_FOREACH_SAFE(_events, ev, tmp) {
    if (ev->context == hContext) {
      DL_DELETE(_events, ev);
      DL_APPEND(evs, ev);
    }
  }
  pthread_mutex_unlock(&_cuda_events_mutex);

  DL_FOREACH_SAFE(evs, ev, tmp) {
    DL_DELETE(evs, ev);
    _profile_event_results(ev);
    CU_EVENT_DESTROY_V2_PTR(ev->start);
    CU_EVENT_DESTROY_V2_PTR(ev->stop);
    free(ev);
  }
}

static void _event_cleanup() {
  struct _cuda_event_s *ev, *tmp;
  DL_FOREACH_SAFE(_events, ev, tmp) {
    DL_DELETE(_events, ev);
    _profile_event_results(ev);
    CU_EVENT_DESTROY_V2_PTR(ev->start);
    CU_EVENT_DESTROY_V2_PTR(ev->stop);
    free(ev);
  }
}

static pthread_mutex_t _primary_contexts_mutex = PTHREAD_MUTEX_INITIALIZER;

struct _primary_contexts_s {
  CUdevice dev;
  UT_hash_handle hh;
  CUcontext ctx;
  unsigned int refcount;
};

struct _primary_contexts_s * _primary_contexts = NULL;

static inline void _primary_context_retain(CUdevice dev, CUcontext ctx) {
  struct _primary_contexts_s * p_c = NULL;

  pthread_mutex_lock(&_primary_contexts_mutex);
  HASH_FIND_INT(_primary_contexts, &dev, p_c);
  if (p_c != NULL && ctx == p_c->ctx) {
    p_c->refcount++;
  } else {
    p_c = (struct _primary_contexts_s *)calloc(1, sizeof(struct _primary_contexts_s));
    p_c->dev = dev;
    p_c->ctx = ctx;
    p_c->refcount = 1;
    HASH_ADD_INT(_primary_contexts, dev, p_c);
  }
  pthread_mutex_unlock(&_primary_contexts_mutex);
}

static inline void _primary_context_release(CUdevice dev) {
  struct _primary_contexts_s * p_c = NULL;

  pthread_mutex_lock(&_primary_contexts_mutex);
  HASH_FIND_INT(_primary_contexts, &dev, p_c);
  if (p_c != NULL) {
    p_c->refcount--;
    if (0 == p_c->refcount) {
      HASH_DEL(_primary_contexts, p_c);
      _context_event_cleanup(p_c->ctx);
      pthread_mutex_unlock(&_primary_contexts_mutex);
      free(p_c);
    } else
      pthread_mutex_unlock(&_primary_contexts_mutex);
  } else
    pthread_mutex_unlock(&_primary_contexts_mutex);
}

static inline void _primary_context_reset(CUdevice dev) {
  struct _primary_contexts_s * p_c = NULL;

  pthread_mutex_lock(&_primary_contexts_mutex);
  HASH_FIND_INT(_primary_contexts, &dev, p_c);
  if (p_c != NULL) {
    _context_event_cleanup(p_c->ctx);
  }
  pthread_mutex_unlock(&_primary_contexts_mutex);
}

static int _trace_from_start() {
  static int trace_from_start = -1;
  if (trace_from_start == -1) {
    const char *env_value = getenv("LTTNG_UST_TRACE_FROM_START");
    trace_from_start = (env_value == NULL || strcmp(env_value, "1") == 0);
  }
  return trace_from_start;
}

static void THAPI_ATTRIBUTE_DESTRUCTOR
_lib_cleanup() {
  if (_do_cleanup) {
    if (_do_profile) {
      _event_cleanup();
    }
  }
}

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  thapi_sampling_init();

  s = getenv("LTTNG_UST_CUDA_LIBCUDA");
  if (s)
    handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  else
    handle = dlopen("libcuda.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  if (handle) {
    void* ptr = dlsym(handle, "cuInit");
    if (ptr == (void*)&cuInit) { //opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if( !handle ) {
    fprintf(stderr, "THAPI: Failure: could not load cuda library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_CUDA_VERBOSE");
  if (s)
    verbose = 1;

  find_cuda_symbols(handle, verbose);
  find_cuda_extensions();

  s = getenv("LTTNG_UST_CUDA_PROFILE");
  if (s)
    _do_profile = 1;
  if(tracepoint_enabled(lttng_ust_cuda_exports, export_called))
    _do_trace_export_tables = 1;

  _do_cleanup = 1;
#ifndef THAPI_USE_DESTRUCTORS
  atexit(_lib_cleanup);
#endif
}

static pthread_once_t _init_tracer_once = PTHREAD_ONCE_INIT;
static __thread volatile int _in_init = 0;
static volatile int _initialized = 0;

static void _init_tracer(void) {
  if( __builtin_expect (_initialized, 1) )
    return;
  /* Avoid reentrancy */
  if (!_in_init) {
    _in_init = 1;
    __sync_synchronize();
    pthread_once(&_init_tracer_once, _load_tracer);
    __sync_synchronize();
    _in_init = 0;
  }
  _initialized = 1;
}

static void _load_cuda(void) {
  _dump_properties();
}

static pthread_once_t _init_cuda_once = PTHREAD_ONCE_INIT;
static __thread volatile int _in_init_cuda = 0;
static volatile int _initialized_cuda = 0;

static void _init_cuda(void) {
  if( __builtin_expect (_initialized_cuda, 1) )
    return;
  if (!_in_init_cuda) {
    _in_init_cuda = 1;
    __sync_synchronize();
    pthread_once(&_init_cuda_once, _load_cuda);
    __sync_synchronize();
    _in_init_cuda = 0;
  }
  _initialized_cuda = 1;
}
