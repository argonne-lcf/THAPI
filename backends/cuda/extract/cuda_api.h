#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#define __CUDA_API_VERSION_INTERNAL=1
#define THAPI_NO_INCLUDE
#include <cuda.h>
typedef int32_t VdpStatus;
typedef uint32_t VdpFuncId;
typedef uint32_t VdpDevice;
typedef VdpStatus  VdpGetProcAddress(
     VdpDevice device,
     VdpFuncId function_id,
     void * *  function_pointer
);
typedef uint32_t VdpVideoSurface;
typedef uint32_t VdpOutputSurface;
#include <cudaVDPAU.h>
#include <cudaProfiler.h>
