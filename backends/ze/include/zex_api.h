// Hand transcription of the compute-runtime experimental (zex) API.
// See README.md.
//
// https://github.com/intel/compute-runtime/tree/master/level_zero/include/level_zero/driver_experimental/
#ifndef _ZEX_API_H
#define _ZEX_API_H
#if defined(__cplusplus)
#pragma once
#endif

#include <ze_api.h>

#if defined(__cplusplus)
extern "C" {
#endif

// --- zex_common.h -----------------------------------------------------------

typedef uint32_t zex_mem_action_scope_flags_t;
typedef enum _zex_mem_action_scope_flag_t {
    ZEX_MEM_ACTION_SCOPE_FLAG_SUBDEVICE = 1,
    ZEX_MEM_ACTION_SCOPE_FLAG_DEVICE = 2,
    ZEX_MEM_ACTION_SCOPE_FLAG_HOST = 4,
    ZEX_MEM_ACTION_SCOPE_FLAG_FORCE_UINT32 = 0x7fffffff
} zex_mem_action_scope_flag_t;

typedef uint32_t zex_wait_on_mem_action_flags_t;
typedef enum _zex_wait_on_mem_action_flag_t {
    ZEX_WAIT_ON_MEMORY_FLAG_EQUAL = 1,
    ZEX_WAIT_ON_MEMORY_FLAG_NOT_EQUAL = 2,
    ZEX_WAIT_ON_MEMORY_FLAG_GREATER_THAN = 4,
    ZEX_WAIT_ON_MEMORY_FLAG_GREATER_THAN_EQUAL = 8,
    ZEX_WAIT_ON_MEMORY_FLAG_LESSER_THAN = 16,
    ZEX_WAIT_ON_MEMORY_FLAG_LESSER_THAN_EQUAL = 32,
    ZEX_WAIT_ON_MEMORY_FLAG_FORCE_UINT32 = 0x7fffffff
} zex_wait_on_mem_action_flag_t;

typedef struct _zex_wait_on_mem_desc_t {
    zex_wait_on_mem_action_flags_t actionFlag;
    zex_mem_action_scope_flags_t waitScope;
} zex_wait_on_mem_desc_t;

typedef struct _zex_write_to_mem_desc_t {
    zex_mem_action_scope_flags_t writeScope;
} zex_write_to_mem_desc_t;

typedef struct _zex_ipc_counter_based_event_handle_t {
    char data[ZE_MAX_IPC_HANDLE_SIZE];
} zex_ipc_counter_based_event_handle_t;

// --- zex_module.h -----------------------------------------------------------

typedef struct _zex_device_module_register_file_exp_t {
    ze_structure_type_t stype;
    const void *pNext;
    uint32_t registerFileSizesCount;
    uint32_t *registerFileSizes;
} zex_device_module_register_file_exp_t;

ZE_APIEXPORT ze_result_t ZE_APICALL
zexKernelGetBaseAddress(
    ze_kernel_handle_t hKernel,
    uint64_t *baseAddress);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexKernelGetArgumentSize(
    ze_kernel_handle_t hKernel,
    uint32_t argIndex,
    uint32_t *pArgSize);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexKernelGetArgumentType(
    ze_kernel_handle_t hKernel,
    uint32_t argIndex,
    uint32_t *pSize,
    char *pString);

// --- zex_driver.h -----------------------------------------------------------

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

// --- zex_memory.h -----------------------------------------------------------

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

// --- zex_cmdlist.h ----------------------------------------------------------

ZE_APIEXPORT ze_result_t ZE_APICALL
zexCommandListAppendWaitOnMemory(
    ze_command_list_handle_t hCommandList,
    zex_wait_on_mem_desc_t *desc,
    void *ptr,
    uint32_t data,
    ze_event_handle_t hSignalEvent);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexCommandListAppendWriteToMemory(
    ze_command_list_handle_t hCommandList,
    zex_write_to_mem_desc_t *desc,
    void *ptr,
    uint64_t data);

// --- zex_event.h ------------------------------------------------------------

ZE_APIEXPORT ze_result_t ZE_APICALL
zexEventGetDeviceAddress(
    ze_event_handle_t event,
    uint64_t *completionValue,
    uint64_t *address);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexCounterBasedEventCreate(
    ze_context_handle_t hContext,
    ze_device_handle_t hDevice,
    uint64_t *deviceAddress,
    uint64_t *hostAddress,
    uint64_t completionValue,
    const ze_event_desc_t *desc,
    ze_event_handle_t *phEvent);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexCounterBasedEventGetIpcHandle(
    ze_event_handle_t hEvent,
    zex_ipc_counter_based_event_handle_t *phIpc);

// zexCounterBasedEventOpenIpcHandle is deliberately absent: it takes a
// 64-byte zex_ipc_counter_based_event_handle_t *by value*, which is class
// MEMORY under the x86-64 SysV ABI (passed on the stack). The libffi closures
// the zex tracer builds describe only scalars and pointers, so it cannot be
// forwarded correctly, and declaring the parameter as a pointer would change
// the ABI and misread the caller's arguments.

ZE_APIEXPORT ze_result_t ZE_APICALL
zexCounterBasedEventCloseIpcHandle(
    ze_event_handle_t hEvent);

// zexIntelAllocateNetworkInterrupt takes `uint32_t &` upstream, a C++
// reference; transcribed as the pointer it is at the ABI level.
ZE_APIEXPORT ze_result_t ZE_APICALL
zexIntelAllocateNetworkInterrupt(
    ze_context_handle_t hContext,
    uint32_t *networkInterruptId);

ZE_APIEXPORT ze_result_t ZE_APICALL
zexIntelReleaseNetworkInterrupt(
    ze_context_handle_t hContext,
    uint32_t networkInterruptId);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_API_H
