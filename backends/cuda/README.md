# CUDA Backend

## VDPAU Stub

The CUDA SDK includes interop headers (`cudaVDPAU.h`, `cuda_vdpau_interop.h`) that reference VDPAU types (`VdpDevice`, `VdpGetProcAddress`, etc.) from `<vdpau/vdpau.h>`. This system header is not always available.

A stub is provided in `include/vdpau/vdpau.h` that defines the necessary VDPAU types so that the CUDA tracer can compiled. 
