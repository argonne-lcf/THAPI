extern __host__ cudaError_t CUDARTAPI __cudaPopCallConfiguration(
  dim3         *gridDim,
  dim3         *blockDim,
  size_t       *sharedMem,
  cudaStream_t *stream
);

extern __host__ cudaError_t CUDARTAPI __cudaPushCallConfiguration(
  dim3         gridDim,
  dim3         blockDim,
  size_t       sharedMem,
  cudaStream_t stream
);

extern __host__ void** CUDARTAPI __cudaRegisterFatBinary(
  void *fatCubin
);

extern __host__ void CUDARTAPI __cudaRegisterFatBinaryEnd(
  void **fatCubinHandle
);

extern __host__ void CUDARTAPI __cudaUnregisterFatBinary(
  void **fatCubinHandle
);

extern __host__ cudaError_t CUDARTAPI __cudaGetKernel(
  void **pKernelRet,
  void  *pFun);

extern __host__ cudaError_t CUDARTAPI __cudaLaunchKernel_ptsz(
  const void          *func,
        dim3           gridDim,
        dim3           blockDim,
        void         **args,
        size_t         sharedMem,
        cudaStream_t   stream);

extern __host__ cudaError_t CUDARTAPI __cudaLaunchKernel(
  const void          *func,
        dim3           gridDim,
        dim3           blockDim,
        void         **args,
        size_t         sharedMem,
        cudaStream_t   stream);

extern __host__ void CUDARTAPI __cudaRegisterHostVar(
        void **fatCubinHandle,
  const char  *deviceName,
        char  *hostVar,
        size_t size);

extern __host__ void CUDARTAPI __cudaRegisterVar(
        void **fatCubinHandle,
        char  *hostVar,
        char  *deviceAddress,
  const char  *deviceName,
        int    ext,
        size_t size,
        int    constant,
        int    global
);

extern __host__ void CUDARTAPI __cudaRegisterManagedVar(
        void **fatCubinHandle,
        void **hostVarPtrAddress,
        char  *deviceAddress,
  const char  *deviceName,
        int    ext,
        size_t size,
        int    constant,
        int    global
);

extern __host__ char CUDARTAPI __cudaInitModule(
        void **fatCubinHandle
);

/*
extern __host__ void CUDARTAPI __cudaRegisterTexture(
        void                    **fatCubinHandle,
  const void                     *hostVar,
  const void                    **deviceAddress,
  const char                     *deviceName,
        int                       dim,
        int                       norm,
        int                        ext
);

extern __host__ void CUDARTAPI __cudaRegisterSurface(
        void                    **fatCubinHandle,
  const void                     *hostVar,
  const void                    **deviceAddress,
  const char                     *deviceName,
        int                       dim,
        int                       ext
);
*/

extern __host__ void CUDARTAPI __cudaRegisterFunction(
        void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize
);

extern __host__ void __cudaRegisterUnifiedTable(
  void *table,
  void *ptr1,
  void *ptr2,
  void *ptr3,
  void *ptr4
);


