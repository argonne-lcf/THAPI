#ifndef HIP_MISSING_APIS_H
#define HIP_MISSING_APIS_H

hipError_t hipGraphDebugDotPrint(hipGraph_t graph, const char *fName, unsigned int flag);

extern void **__hipRegisterFatBinary(const void *data);

extern void __hipRegisterFunction(void **modules,
                                  const void *hostFunction,
                                  char *deviceFunction,
                                  const char *deviceName,
                                  unsigned int threadLimit,
                                  uint3 *tid,
                                  uint3 *bid,
                                  dim3 *blockDim,
                                  dim3 *gridDim,
                                  int *wSize);

extern void __hipRegisterManagedVar(void **modules,
                                    void **pointer,
                                    void *init_value,
                                    const char *name,
                                    size_t size,
                                    unsigned align);

extern void
__hipRegisterSurface(void **modules, void *var, char *hostVar, char *deviceVar, int type, int ext);

extern void __hipRegisterTexture(
    void **modules, void *var, char *hostVar, char *deviceVar, int type, int norm, int ext);

extern void __hipRegisterVar(void **modules,
                             void *var,
                             char *hostVar,
                             char *deviceVar,
                             int ext,
                             size_t size,
                             int constant,
                             int global);

extern void __hipUnregisterFatBinary(void **modules);

extern const char *hipGetCmdName(uint32_t id);

typedef enum activity_domain_t {
  ACTIVITY_DOMAIN_HSA_API = 0,
  ACTIVITY_DOMAIN_HSA_OPS = 1,
  ACTIVITY_DOMAIN_HIP_OPS = 2,
  ACTIVITY_DOMAIN_HCC_OPS = ACTIVITY_DOMAIN_HIP_OPS,
  ACTIVITY_DOMAIN_HIP_VDI = ACTIVITY_DOMAIN_HIP_OPS,
  ACTIVITY_DOMAIN_HIP_API = 3,
  ACTIVITY_DOMAIN_KFD_API = 4,
  ACTIVITY_DOMAIN_EXT_API = 5,
  ACTIVITY_DOMAIN_ROCTX = 6,
  ACTIVITY_DOMAIN_HSA_EVT = 7,
  ACTIVITY_DOMAIN_NUMBER
} activity_domain_t;

typedef int
hipRegisterTracerCallback_callback_t(activity_domain_t domain, uint32_t operation_id, void *data);

extern void hipRegisterTracerCallback(hipRegisterTracerCallback_callback_t *function);

#endif
