// compute-runtime/level_zero/core/source/driver/extension_function_address.cpp
#ifndef _ZEX_API_H
#define _ZEX_API_H
#if defined(__cplusplus)
#pragma once
#endif

#include <ze_api.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _zex_device_module_register_file_exp_t {
    ze_structure_type_t stype;
    const void *pNext;
    uint32_t registerFileSizesCount;
    uint32_t *registerFileSizes;
} zex_device_module_register_file_exp_t;

ZE_APIEXPORT ze_result_t ZE_APICALL
zexDriverImportExternalPointer(
    ze_driver_handle_t hDriver,
    void *ptr,
    size_t size);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexDriverReleaseImportedPointer(
    ze_driver_handle_t hDriver,
    void *ptr);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexDriverGetHostPointerBaseAddress(
    ze_driver_handle_t hDriver,
    void *ptr,
    void **baseAddress);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexKernelGetBaseAddress(
    ze_kernel_handle_t hKernel,
    uint64_t *baseAddress);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexMemGetIpcHandles(
    ze_context_handle_t hContext,
    const void *ptr,
    uint32_t *numIpcHandles,
    ze_ipc_mem_handle_t *pIpcHandles);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexMemOpenIpcHandles(
    ze_context_handle_t hContext,
    ze_device_handle_t hDevice,
    uint32_t numIpcHandles,
    ze_ipc_mem_handle_t *pIpcHandles,
    ze_ipc_memory_flags_t flags,
    void **pptr);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_API_H
