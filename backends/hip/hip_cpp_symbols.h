void
_ZN8hip_impl22hipLaunchKernelGGLImplEmRK4dim3S2_jP12ihipStream_tPPv(
	uintptr_t function_address, const dim3 *numBlocks, const dim3 *dimBlocks,
	uint32_t sharedMemBytes, hipStream_t stream, void** kernarg);
hipError_t
_Z22hipCreateSurfaceObjectPP13__hip_surfacePK15hipResourceDesc(
	hipSurfaceObject_t* pSurfObject, const hipResourceDesc* pResDesc);
hipError_t
_Z23hipDestroySurfaceObjectP13__hip_surface(
	hipSurfaceObject_t surfaceObject);
hipError_t
_Z24hipHccModuleLaunchKernelP18ihipModuleSymbol_tjjjjjjmP12ihipStream_tPPvS4_P11ihipEvent_tS6_(
	hipFunction_t f, uint32_t globalWorkSizeX,
	uint32_t globalWorkSizeY, uint32_t globalWorkSizeZ,
	uint32_t localWorkSizeX, uint32_t localWorkSizeY,
        uint32_t localWorkSizeZ, size_t sharedMemBytes,
        hipStream_t hStream, void** kernelParams, void** extra,
        hipEvent_t startEvent,
        hipEvent_t stopEvent);
hipError_t
_Z24hipExtModuleLaunchKernelP18ihipModuleSymbol_tjjjjjjmP12ihipStream_tPPvS4_P11ihipEvent_tS6_j(
	hipFunction_t f, uint32_t globalWorkSizeX,
	uint32_t globalWorkSizeY, uint32_t globalWorkSizeZ,
	uint32_t localWorkSizeX, uint32_t localWorkSizeY,
        uint32_t localWorkSizeZ, size_t sharedMemBytes,
        hipStream_t hStream, void** kernelParams, void** extra,
        hipEvent_t startEvent,
        hipEvent_t stopEvent,
	uint32_t flags);

