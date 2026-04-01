/* Stub for VDPAU types used by CUDA interop headers. */
#ifndef THAPI_VDPAU_STUB_H
#define THAPI_VDPAU_STUB_H

#include <stdint.h>

typedef int32_t VdpStatus;
typedef uint32_t VdpFuncId;
typedef uint32_t VdpDevice;
typedef VdpStatus VdpGetProcAddress(VdpDevice device, VdpFuncId function_id,
                                    void **function_pointer);
typedef uint32_t VdpVideoSurface;
typedef uint32_t VdpOutputSurface;

#endif
