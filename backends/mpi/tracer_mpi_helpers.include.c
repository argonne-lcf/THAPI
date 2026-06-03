static pthread_once_t _init = PTHREAD_ONCE_INIT;
static __thread volatile int in_init = 0;
static volatile unsigned int _initialized = 0;

/* ---------------------------------------------------------------------------
 * GPU pointer introspection
 *
 * Goal: when an MPI call carries a buffer pointer, decide at runtime whether
 * the pointer refers to host memory or to a GPU allocation, so that downstream
 * tools can tag the call as "GPU-aware MPI" vs "CPU-only MPI".
 *
 * Design constraints (matching the existing MPI backend):
 *   - No build-time dependency on Level Zero / CUDA / HIP.  Each loader is
 *     dlopen'd lazily, and only the symbols we need are looked up via dlsym
 *     using locally-declared function-pointer signatures over opaque types.
 *   - All overhead is gated by tracepoint_enabled() and a small per-thread
 *     cache keyed on the allocation base address, so a buffer reused across
 *     iterations is classified only once.
 * ------------------------------------------------------------------------- */

/* Values reported in the buffer_info tracepoint.  Keep in sync with consumers
 * (extract/, btx_mpiinterval_callbacks.cpp). */
enum thapi_mpi_ptr_kind {
  THAPI_MPI_PTR_UNKNOWN = 0,
  THAPI_MPI_PTR_HOST    = 1,
  THAPI_MPI_PTR_DEVICE  = 2,
  THAPI_MPI_PTR_SHARED  = 3
};

enum thapi_mpi_ptr_backend {
  THAPI_MPI_BACKEND_NONE = 0,
  THAPI_MPI_BACKEND_L0   = 1,
  THAPI_MPI_BACKEND_CUDA = 2,
  THAPI_MPI_BACKEND_HIP  = 3
};

enum thapi_mpi_buffer_role {
  THAPI_MPI_ROLE_IN  = 0,
  THAPI_MPI_ROLE_OUT = 1
};

/* --- Level Zero ----------------------------------------------------------- */
/* Opaque handle types and the minimal enums/structs needed to call
 * zeMemGetAllocProperties / zeMemGetAddressRange without including ze_api.h. */
typedef int                        _thapi_ze_result_t;     /* ze_result_t */
typedef struct _thapi_ze_driver   *_thapi_ze_driver_handle_t;
typedef struct _thapi_ze_context  *_thapi_ze_context_handle_t;
typedef struct _thapi_ze_device   *_thapi_ze_device_handle_t;

#define _THAPI_ZE_RESULT_SUCCESS                          0
#define _THAPI_ZE_STRUCTURE_TYPE_MEM_ALLOC_PROPERTIES 0x000c
#define _THAPI_ZE_INIT_FLAG_GPU_ONLY                   0x01

typedef enum {
  _THAPI_ZE_MEMORY_TYPE_UNKNOWN = 0,
  _THAPI_ZE_MEMORY_TYPE_HOST    = 1,
  _THAPI_ZE_MEMORY_TYPE_DEVICE  = 2,
  _THAPI_ZE_MEMORY_TYPE_SHARED  = 3
} _thapi_ze_memory_type_t;

typedef struct {
  uint32_t                stype;
  const void             *pNext;
  _thapi_ze_memory_type_t type;
  uint64_t                id;
  uint64_t                pageSize;
} _thapi_ze_memory_allocation_properties_t;

typedef _thapi_ze_result_t (*_thapi_ze_init_fn_t)(uint32_t flags);
typedef _thapi_ze_result_t (*_thapi_ze_driver_get_fn_t)(uint32_t *pCount,
                                                       _thapi_ze_driver_handle_t *phDrivers);
typedef _thapi_ze_result_t (*_thapi_ze_context_create_fn_t)(_thapi_ze_driver_handle_t hDriver,
                                                            const void *desc,
                                                            _thapi_ze_context_handle_t *phContext);
typedef _thapi_ze_result_t (*_thapi_ze_mem_get_alloc_properties_fn_t)(
    _thapi_ze_context_handle_t hContext, const void *ptr,
    _thapi_ze_memory_allocation_properties_t *pProps,
    _thapi_ze_device_handle_t *phDevice);
typedef _thapi_ze_result_t (*_thapi_ze_mem_get_address_range_fn_t)(
    _thapi_ze_context_handle_t hContext, const void *ptr,
    void **pBase, size_t *pSize);

static _thapi_ze_mem_get_alloc_properties_fn_t _ze_mem_get_alloc_properties = NULL;
static _thapi_ze_mem_get_address_range_fn_t    _ze_mem_get_address_range    = NULL;
static _thapi_ze_context_handle_t              _ze_introspection_ctx        = NULL;

/* --- CUDA ----------------------------------------------------------------- */
/* Driver API (libcuda.so) is preferred: it's the same on every CUDA install
 * and doesn't drag in libcudart. */
typedef int          _thapi_cu_result_t;          /* CUresult */
typedef unsigned int _thapi_cu_attr_t;            /* CUpointer_attribute */
typedef unsigned long long _thapi_cu_deviceptr_t; /* CUdeviceptr */

#define _THAPI_CU_SUCCESS                        0
#define _THAPI_CU_POINTER_ATTRIBUTE_MEMORY_TYPE  2
#define _THAPI_CU_POINTER_ATTRIBUTE_RANGE_START  7
#define _THAPI_CU_POINTER_ATTRIBUTE_RANGE_SIZE   8
#define _THAPI_CU_POINTER_ATTRIBUTE_DEVICE_ORDINAL 9
#define _THAPI_CU_MEMORYTYPE_HOST                1
#define _THAPI_CU_MEMORYTYPE_DEVICE              2
#define _THAPI_CU_MEMORYTYPE_ARRAY               3
#define _THAPI_CU_MEMORYTYPE_UNIFIED             4

typedef _thapi_cu_result_t (*_thapi_cu_init_fn_t)(unsigned int flags);
typedef _thapi_cu_result_t (*_thapi_cu_pointer_get_attribute_fn_t)(void *data,
                                                                   _thapi_cu_attr_t attr,
                                                                   _thapi_cu_deviceptr_t ptr);

static _thapi_cu_pointer_get_attribute_fn_t _cu_pointer_get_attribute = NULL;

/* --- HIP ------------------------------------------------------------------ */
typedef int _thapi_hip_error_t; /* hipError_t */

#define _THAPI_HIP_SUCCESS               0
#define _THAPI_HIP_MEMORY_TYPE_HOST      0
#define _THAPI_HIP_MEMORY_TYPE_DEVICE    1
#define _THAPI_HIP_MEMORY_TYPE_ARRAY     2
#define _THAPI_HIP_MEMORY_TYPE_UNIFIED   3
#define _THAPI_HIP_MEMORY_TYPE_MANAGED   4

typedef struct {
  unsigned int  type;
  int           device;
  void         *devicePointer;
  void         *hostPointer;
  int           isManaged;
  unsigned int  allocationFlags;
} _thapi_hip_pointer_attribute_t;

typedef _thapi_hip_error_t (*_thapi_hip_pointer_get_attributes_fn_t)(
    _thapi_hip_pointer_attribute_t *attr, const void *ptr);
typedef _thapi_hip_error_t (*_thapi_hip_mem_get_address_range_fn_t)(
    void **pbase, size_t *psize, void *dptr);

static _thapi_hip_pointer_get_attributes_fn_t _hip_pointer_get_attributes = NULL;
static _thapi_hip_mem_get_address_range_fn_t  _hip_mem_get_address_range  = NULL;

/* --- Per-thread address-range cache --------------------------------------- */
/* Tiny LRU keyed on allocation base address.  Buffers reused across MPI
 * iterations (the common case in HPC) hit the cache and skip the runtime
 * query entirely.  Sized small enough to live cheaply per-thread. */
#define _THAPI_MPI_CACHE_SLOTS 16

struct _thapi_mpi_cache_entry {
  uintptr_t base;
  size_t    size;
  uint8_t   kind;
  uint8_t   backend;
  uint32_t  device_ordinal;
  uint8_t   valid;
};

static __thread struct _thapi_mpi_cache_entry
    _thapi_mpi_cache[_THAPI_MPI_CACHE_SLOTS];
static __thread unsigned int _thapi_mpi_cache_next = 0;

static inline int _thapi_mpi_cache_lookup(const void *ptr,
                                          struct _thapi_mpi_cache_entry *out) {
  uintptr_t p = (uintptr_t)ptr;
  for (unsigned int i = 0; i < _THAPI_MPI_CACHE_SLOTS; i++) {
    struct _thapi_mpi_cache_entry *e = &_thapi_mpi_cache[i];
    if (e->valid && p >= e->base && p < e->base + e->size) {
      *out = *e;
      return 1;
    }
  }
  return 0;
}

static inline void _thapi_mpi_cache_insert(const struct _thapi_mpi_cache_entry *e) {
  _thapi_mpi_cache[_thapi_mpi_cache_next] = *e;
  _thapi_mpi_cache_next =
      (_thapi_mpi_cache_next + 1) % _THAPI_MPI_CACHE_SLOTS;
}

/* --- Runtime probing ------------------------------------------------------ */
static int _try_classify_ze(const void *ptr,
                            struct _thapi_mpi_cache_entry *out) {
  if (!_ze_mem_get_alloc_properties || !_ze_introspection_ctx)
    return 0;

  _thapi_ze_memory_allocation_properties_t props;
  props.stype = _THAPI_ZE_STRUCTURE_TYPE_MEM_ALLOC_PROPERTIES;
  props.pNext = NULL;
  props.type  = _THAPI_ZE_MEMORY_TYPE_UNKNOWN;
  _thapi_ze_device_handle_t hDev = NULL;

  if (_ze_mem_get_alloc_properties(_ze_introspection_ctx, ptr, &props, &hDev)
      != _THAPI_ZE_RESULT_SUCCESS)
    return 0;
  if (props.type == _THAPI_ZE_MEMORY_TYPE_UNKNOWN)
    return 0;

  void  *base = NULL;
  size_t size = 0;
  if (_ze_mem_get_address_range &&
      _ze_mem_get_address_range(_ze_introspection_ctx, ptr, &base, &size)
          != _THAPI_ZE_RESULT_SUCCESS) {
    base = (void *)ptr;
    size = 1;
  }

  out->base = (uintptr_t)base;
  out->size = size ? size : 1;
  out->backend = THAPI_MPI_BACKEND_L0;
  out->device_ordinal = 0; /* L0 device ordinal lookup would need extra calls */
  switch (props.type) {
    case _THAPI_ZE_MEMORY_TYPE_HOST:   out->kind = THAPI_MPI_PTR_HOST;   break;
    case _THAPI_ZE_MEMORY_TYPE_DEVICE: out->kind = THAPI_MPI_PTR_DEVICE; break;
    case _THAPI_ZE_MEMORY_TYPE_SHARED: out->kind = THAPI_MPI_PTR_SHARED; break;
    default:                           out->kind = THAPI_MPI_PTR_UNKNOWN; break;
  }
  out->valid = 1;
  return 1;
}

static int _try_classify_cuda(const void *ptr,
                              struct _thapi_mpi_cache_entry *out) {
  if (!_cu_pointer_get_attribute)
    return 0;

  unsigned int           mem_type = 0;
  _thapi_cu_deviceptr_t  range_start = 0;
  size_t                 range_size = 0;
  int                    device_ordinal = 0;
  _thapi_cu_deviceptr_t  dptr = (_thapi_cu_deviceptr_t)(uintptr_t)ptr;

  if (_cu_pointer_get_attribute(&mem_type,
                                _THAPI_CU_POINTER_ATTRIBUTE_MEMORY_TYPE, dptr)
      != _THAPI_CU_SUCCESS || mem_type == 0)
    return 0;

  (void)_cu_pointer_get_attribute(&range_start,
                                  _THAPI_CU_POINTER_ATTRIBUTE_RANGE_START, dptr);
  (void)_cu_pointer_get_attribute(&range_size,
                                  _THAPI_CU_POINTER_ATTRIBUTE_RANGE_SIZE, dptr);
  (void)_cu_pointer_get_attribute(&device_ordinal,
                                  _THAPI_CU_POINTER_ATTRIBUTE_DEVICE_ORDINAL, dptr);

  out->base = range_start ? (uintptr_t)range_start : (uintptr_t)ptr;
  out->size = range_size ? range_size : 1;
  out->backend = THAPI_MPI_BACKEND_CUDA;
  out->device_ordinal = (uint32_t)device_ordinal;
  switch (mem_type) {
    case _THAPI_CU_MEMORYTYPE_HOST:    out->kind = THAPI_MPI_PTR_HOST;   break;
    case _THAPI_CU_MEMORYTYPE_DEVICE:  out->kind = THAPI_MPI_PTR_DEVICE; break;
    case _THAPI_CU_MEMORYTYPE_UNIFIED: out->kind = THAPI_MPI_PTR_SHARED; break;
    default:                           out->kind = THAPI_MPI_PTR_UNKNOWN; break;
  }
  out->valid = 1;
  return 1;
}

static int _try_classify_hip(const void *ptr,
                             struct _thapi_mpi_cache_entry *out) {
  if (!_hip_pointer_get_attributes)
    return 0;

  _thapi_hip_pointer_attribute_t attr;
  memset(&attr, 0, sizeof(attr));
  if (_hip_pointer_get_attributes(&attr, ptr) != _THAPI_HIP_SUCCESS)
    return 0;

  void  *base = (void *)ptr;
  size_t size = 1;
  if (_hip_mem_get_address_range)
    (void)_hip_mem_get_address_range(&base, &size, (void *)ptr);

  out->base = (uintptr_t)base;
  out->size = size ? size : 1;
  out->backend = THAPI_MPI_BACKEND_HIP;
  out->device_ordinal = (uint32_t)attr.device;
  if (attr.isManaged || attr.type == _THAPI_HIP_MEMORY_TYPE_UNIFIED ||
      attr.type == _THAPI_HIP_MEMORY_TYPE_MANAGED)
    out->kind = THAPI_MPI_PTR_SHARED;
  else if (attr.type == _THAPI_HIP_MEMORY_TYPE_DEVICE)
    out->kind = THAPI_MPI_PTR_DEVICE;
  else if (attr.type == _THAPI_HIP_MEMORY_TYPE_HOST)
    out->kind = THAPI_MPI_PTR_HOST;
  else
    out->kind = THAPI_MPI_PTR_UNKNOWN;
  out->valid = 1;
  return 1;
}

static inline void _dump_buffer_info(const void *ptr, int role) {
  if (!ptr)
    return;
  /* MPI_IN_PLACE / MPI_BOTTOM are sentinel values, not real pointers. */
  if (ptr == MPI_IN_PLACE || ptr == MPI_BOTTOM)
    return;
  if (!tracepoint_enabled(lttng_ust_mpi_properties, buffer_info))
    return;

  struct _thapi_mpi_cache_entry e;
  if (!_thapi_mpi_cache_lookup(ptr, &e)) {
    e.valid = 0;
    if (!_try_classify_ze(ptr, &e) &&
        !_try_classify_cuda(ptr, &e) &&
        !_try_classify_hip(ptr, &e)) {
      /* Nobody claims it: treat as host. Cache a 1-byte range so we still
       * answer in O(1) on the next call, but don't pollute the cache with
       * adjacent host allocations. */
      e.base = (uintptr_t)ptr;
      e.size = 1;
      e.kind = THAPI_MPI_PTR_HOST;
      e.backend = THAPI_MPI_BACKEND_NONE;
      e.device_ordinal = 0;
      e.valid = 1;
    }
    _thapi_mpi_cache_insert(&e);
  }

  do_tracepoint(lttng_ust_mpi_properties, buffer_info,
                ptr, role, (int)e.kind, (int)e.backend,
                e.base, (uint64_t)e.size, e.device_ordinal);
}

/* --- Loader -------------------------------------------------------------- */
static void _load_gpu_introspection(int verbose) {
  /* Each loader is optional: if the runtime isn't present on the system, the
   * corresponding classifier stays NULL and is silently skipped. */
  void *h_ze = dlopen("libze_loader.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!h_ze)
    h_ze = dlopen("libze_loader.so",   RTLD_LAZY | RTLD_LOCAL);
  if (h_ze) {
    _thapi_ze_init_fn_t           ze_init    = (_thapi_ze_init_fn_t)
        dlsym(h_ze, "zeInit");
    _thapi_ze_driver_get_fn_t     ze_driver  = (_thapi_ze_driver_get_fn_t)
        dlsym(h_ze, "zeDriverGet");
    _thapi_ze_context_create_fn_t ze_ctx_new = (_thapi_ze_context_create_fn_t)
        dlsym(h_ze, "zeContextCreate");
    _ze_mem_get_alloc_properties = (_thapi_ze_mem_get_alloc_properties_fn_t)
        dlsym(h_ze, "zeMemGetAllocProperties");
    _ze_mem_get_address_range    = (_thapi_ze_mem_get_address_range_fn_t)
        dlsym(h_ze, "zeMemGetAddressRange");

    /* zeMemGetAllocProperties needs a context.  Create a private one against
     * the first driver so introspection never touches application contexts. */
    uint32_t n = 1;
    _thapi_ze_driver_handle_t hDrv = NULL;
    struct { uint32_t stype; const void *pNext; uint32_t flags; } desc = {
        0x000d /* ZE_STRUCTURE_TYPE_CONTEXT_DESC */, NULL, 0 };
    if (ze_init && ze_driver && ze_ctx_new &&
        ze_init(_THAPI_ZE_INIT_FLAG_GPU_ONLY) == _THAPI_ZE_RESULT_SUCCESS &&
        ze_driver(&n, &hDrv) == _THAPI_ZE_RESULT_SUCCESS && hDrv &&
        ze_ctx_new(hDrv, &desc, &_ze_introspection_ctx)
            == _THAPI_ZE_RESULT_SUCCESS) {
      if (verbose)
        fprintf(stderr, "THAPI/MPI: Level Zero pointer introspection enabled\n");
    } else {
      _ze_mem_get_alloc_properties = NULL;
      _ze_mem_get_address_range    = NULL;
      _ze_introspection_ctx        = NULL;
    }
  }

  void *h_cu = dlopen("libcuda.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!h_cu)
    h_cu = dlopen("libcuda.so",   RTLD_LAZY | RTLD_LOCAL);
  if (h_cu) {
    _thapi_cu_init_fn_t cu_init =
        (_thapi_cu_init_fn_t)dlsym(h_cu, "cuInit");
    _cu_pointer_get_attribute = (_thapi_cu_pointer_get_attribute_fn_t)
        dlsym(h_cu, "cuPointerGetAttribute");
    if (cu_init && _cu_pointer_get_attribute &&
        cu_init(0) == _THAPI_CU_SUCCESS) {
      if (verbose)
        fprintf(stderr, "THAPI/MPI: CUDA pointer introspection enabled\n");
    } else {
      _cu_pointer_get_attribute = NULL;
    }
  }

  void *h_hip = dlopen("libamdhip64.so.5", RTLD_LAZY | RTLD_LOCAL);
  if (!h_hip)
    h_hip = dlopen("libamdhip64.so",   RTLD_LAZY | RTLD_LOCAL);
  if (h_hip) {
    _hip_pointer_get_attributes = (_thapi_hip_pointer_get_attributes_fn_t)
        dlsym(h_hip, "hipPointerGetAttributes");
    _hip_mem_get_address_range  = (_thapi_hip_mem_get_address_range_fn_t)
        dlsym(h_hip, "hipMemGetAddressRange");
    if (_hip_pointer_get_attributes && verbose)
      fprintf(stderr, "THAPI/MPI: HIP pointer introspection enabled\n");
  }
}

static void _load_tracer(void) {
  char *s = NULL;
  void *handle = NULL;
  int verbose = 0;

  s = getenv("LTTNG_UST_MPI_LIBMPI");
  if (s)
    handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  else
    handle = dlopen("libmpi.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  if (handle) {
    void *ptr = dlsym(handle, "MPI_Init");
    if (ptr == (void *)&MPI_Init) { // opening oneself
      dlclose(handle);
      handle = NULL;
    }
  }

  if (!handle) {
    fprintf(stderr, "THAPI: Failure: could not load MPI library!\n");
    exit(1);
  }

  s = getenv("LTTNG_UST_MPI_VERBOSE");
  if (s)
    verbose = 1;

  find_mpi_symbols(handle, verbose);
  _load_gpu_introspection(verbose);
}

static inline void _init_tracer(void) {
  if (__builtin_expect(_initialized, 1))
    return;
  /* Avoid reentrancy */
  if (!in_init) {
    in_init = 1;
    __sync_synchronize();
    pthread_once(&_init, _load_tracer);
    __sync_synchronize();
    in_init = 0;
  }
  _initialized = 1;
}
