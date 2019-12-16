/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_ddi.h
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core
 * @endcond
 *
 */
#ifndef _ZE_DDI_H
#define _ZE_DDI_H
#if defined(__cplusplus)
#pragma once
#endif
#include "ze_api.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef ze_result_t (__zecall *ze_pfnInit_t)(
    ze_init_flag_t
    );

typedef struct _ze_global_dditable_t
{
    ze_pfnInit_t                                                pfnInit;
} ze_global_dditable_t;

__zedllexport ze_result_t __zecall
zeGetGlobalProcAddrTable(
    ze_api_version_t version,
    ze_global_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetGlobalProcAddrTable_t)(
    ze_api_version_t,
    ze_global_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGet_t)(
    ze_driver_handle_t,
    uint32_t*,
    ze_device_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetSubDevices_t)(
    ze_device_handle_t,
    uint32_t*,
    ze_device_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetProperties_t)(
    ze_device_handle_t,
    ze_device_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetComputeProperties_t)(
    ze_device_handle_t,
    ze_device_compute_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetMemoryProperties_t)(
    ze_device_handle_t,
    uint32_t*,
    ze_device_memory_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetMemoryAccessProperties_t)(
    ze_device_handle_t,
    ze_device_memory_access_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetCacheProperties_t)(
    ze_device_handle_t,
    ze_device_cache_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetImageProperties_t)(
    ze_device_handle_t,
    ze_device_image_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceGetP2PProperties_t)(
    ze_device_handle_t,
    ze_device_handle_t,
    ze_device_p2p_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceCanAccessPeer_t)(
    ze_device_handle_t,
    ze_device_handle_t,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *ze_pfnDeviceSetLastLevelCacheConfig_t)(
    ze_device_handle_t,
    ze_cache_config_t
    );

typedef ze_result_t (__zecall *ze_pfnDeviceSystemBarrier_t)(
    ze_device_handle_t
    );

#if ZE_ENABLE_OCL_INTEROP
typedef ze_result_t (__zecall *ze_pfnDeviceRegisterCLMemory_t)(
    ze_device_handle_t,
    cl_context,
    cl_mem,
    void**
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef ze_result_t (__zecall *ze_pfnDeviceRegisterCLProgram_t)(
    ze_device_handle_t,
    cl_context,
    cl_program,
    ze_module_handle_t*
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef ze_result_t (__zecall *ze_pfnDeviceRegisterCLCommandQueue_t)(
    ze_device_handle_t,
    cl_context,
    cl_command_queue,
    ze_command_queue_handle_t*
    );
#endif // ZE_ENABLE_OCL_INTEROP

typedef ze_result_t (__zecall *ze_pfnDeviceMakeMemoryResident_t)(
    ze_device_handle_t,
    void*,
    size_t
    );

typedef ze_result_t (__zecall *ze_pfnDeviceEvictMemory_t)(
    ze_device_handle_t,
    void*,
    size_t
    );

typedef ze_result_t (__zecall *ze_pfnDeviceMakeImageResident_t)(
    ze_device_handle_t,
    ze_image_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnDeviceEvictImage_t)(
    ze_device_handle_t,
    ze_image_handle_t
    );

typedef struct _ze_device_dditable_t
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceSetLastLevelCacheConfig_t                       pfnSetLastLevelCacheConfig;
    ze_pfnDeviceSystemBarrier_t                                 pfnSystemBarrier;
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLMemory_t                              pfnRegisterCLMemory;
#endif // ZE_ENABLE_OCL_INTEROP
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLProgram_t                             pfnRegisterCLProgram;
#endif // ZE_ENABLE_OCL_INTEROP
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLCommandQueue_t                        pfnRegisterCLCommandQueue;
#endif // ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceMakeMemoryResident_t                            pfnMakeMemoryResident;
    ze_pfnDeviceEvictMemory_t                                   pfnEvictMemory;
    ze_pfnDeviceMakeImageResident_t                             pfnMakeImageResident;
    ze_pfnDeviceEvictImage_t                                    pfnEvictImage;
} ze_device_dditable_t;

__zedllexport ze_result_t __zecall
zeGetDeviceProcAddrTable(
    ze_api_version_t version,
    ze_device_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetDeviceProcAddrTable_t)(
    ze_api_version_t,
    ze_device_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGet_t)(
    uint32_t*,
    ze_driver_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetDriverVersion_t)(
    ze_driver_handle_t,
    uint32_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetApiVersion_t)(
    ze_driver_handle_t,
    ze_api_version_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetIPCProperties_t)(
    ze_driver_handle_t,
    ze_driver_ipc_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetExtensionFunctionAddress_t)(
    ze_driver_handle_t,
    const char*,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnDriverAllocSharedMem_t)(
    ze_driver_handle_t,
    ze_device_handle_t,
    ze_device_mem_alloc_flag_t,
    uint32_t,
    ze_host_mem_alloc_flag_t,
    size_t,
    size_t,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnDriverAllocDeviceMem_t)(
    ze_driver_handle_t,
    ze_device_handle_t,
    ze_device_mem_alloc_flag_t,
    uint32_t,
    size_t,
    size_t,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnDriverAllocHostMem_t)(
    ze_driver_handle_t,
    ze_host_mem_alloc_flag_t,
    size_t,
    size_t,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnDriverFreeMem_t)(
    ze_driver_handle_t,
    void*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetMemAllocProperties_t)(
    ze_driver_handle_t,
    const void*,
    ze_memory_allocation_properties_t*,
    ze_device_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetMemAddressRange_t)(
    ze_driver_handle_t,
    const void*,
    void**,
    size_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverGetMemIpcHandle_t)(
    ze_driver_handle_t,
    const void*,
    ze_ipc_mem_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnDriverOpenMemIpcHandle_t)(
    ze_driver_handle_t,
    ze_device_handle_t,
    ze_ipc_mem_handle_t,
    ze_ipc_memory_flag_t,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnDriverCloseMemIpcHandle_t)(
    ze_driver_handle_t,
    const void*
    );

typedef struct _ze_driver_dditable_t
{
    ze_pfnDriverGet_t                                           pfnGet;
    ze_pfnDriverGetDriverVersion_t                              pfnGetDriverVersion;
    ze_pfnDriverGetApiVersion_t                                 pfnGetApiVersion;
    ze_pfnDriverGetIPCProperties_t                              pfnGetIPCProperties;
    ze_pfnDriverGetExtensionFunctionAddress_t                   pfnGetExtensionFunctionAddress;
    ze_pfnDriverAllocSharedMem_t                                pfnAllocSharedMem;
    ze_pfnDriverAllocDeviceMem_t                                pfnAllocDeviceMem;
    ze_pfnDriverAllocHostMem_t                                  pfnAllocHostMem;
    ze_pfnDriverFreeMem_t                                       pfnFreeMem;
    ze_pfnDriverGetMemAllocProperties_t                         pfnGetMemAllocProperties;
    ze_pfnDriverGetMemAddressRange_t                            pfnGetMemAddressRange;
    ze_pfnDriverGetMemIpcHandle_t                               pfnGetMemIpcHandle;
    ze_pfnDriverOpenMemIpcHandle_t                              pfnOpenMemIpcHandle;
    ze_pfnDriverCloseMemIpcHandle_t                             pfnCloseMemIpcHandle;
} ze_driver_dditable_t;

__zedllexport ze_result_t __zecall
zeGetDriverProcAddrTable(
    ze_api_version_t version,
    ze_driver_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetDriverProcAddrTable_t)(
    ze_api_version_t,
    ze_driver_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandQueueCreate_t)(
    ze_device_handle_t,
    const ze_command_queue_desc_t*,
    ze_command_queue_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandQueueDestroy_t)(
    ze_command_queue_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandQueueExecuteCommandLists_t)(
    ze_command_queue_handle_t,
    uint32_t,
    ze_command_list_handle_t*,
    ze_fence_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandQueueSynchronize_t)(
    ze_command_queue_handle_t,
    uint32_t
    );

typedef struct _ze_command_queue_dditable_t
{
    ze_pfnCommandQueueCreate_t                                  pfnCreate;
    ze_pfnCommandQueueDestroy_t                                 pfnDestroy;
    ze_pfnCommandQueueExecuteCommandLists_t                     pfnExecuteCommandLists;
    ze_pfnCommandQueueSynchronize_t                             pfnSynchronize;
} ze_command_queue_dditable_t;

__zedllexport ze_result_t __zecall
zeGetCommandQueueProcAddrTable(
    ze_api_version_t version,
    ze_command_queue_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetCommandQueueProcAddrTable_t)(
    ze_api_version_t,
    ze_command_queue_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListCreate_t)(
    ze_device_handle_t,
    const ze_command_list_desc_t*,
    ze_command_list_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListCreateImmediate_t)(
    ze_device_handle_t,
    const ze_command_queue_desc_t*,
    ze_command_list_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListDestroy_t)(
    ze_command_list_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListClose_t)(
    ze_command_list_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListReset_t)(
    ze_command_list_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendBarrier_t)(
    ze_command_list_handle_t,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemoryRangesBarrier_t)(
    ze_command_list_handle_t,
    uint32_t,
    const size_t*,
    const void**,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemoryCopy_t)(
    ze_command_list_handle_t,
    void*,
    const void*,
    size_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemorySet_t)(
    ze_command_list_handle_t,
    void*,
    int,
    size_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemoryCopyRegion_t)(
    ze_command_list_handle_t,
    void*,
    const ze_copy_region_t*,
    uint32_t,
    const void*,
    const ze_copy_region_t*,
    uint32_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendImageCopy_t)(
    ze_command_list_handle_t,
    ze_image_handle_t,
    ze_image_handle_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendImageCopyRegion_t)(
    ze_command_list_handle_t,
    ze_image_handle_t,
    ze_image_handle_t,
    const ze_image_region_t*,
    const ze_image_region_t*,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendImageCopyToMemory_t)(
    ze_command_list_handle_t,
    void*,
    ze_image_handle_t,
    const ze_image_region_t*,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendImageCopyFromMemory_t)(
    ze_command_list_handle_t,
    ze_image_handle_t,
    const void*,
    const ze_image_region_t*,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemoryPrefetch_t)(
    ze_command_list_handle_t,
    const void*,
    size_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendMemAdvise_t)(
    ze_command_list_handle_t,
    ze_device_handle_t,
    const void*,
    size_t,
    ze_memory_advice_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendSignalEvent_t)(
    ze_command_list_handle_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendWaitOnEvents_t)(
    ze_command_list_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendEventReset_t)(
    ze_command_list_handle_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendLaunchKernel_t)(
    ze_command_list_handle_t,
    ze_kernel_handle_t,
    const ze_thread_group_dimensions_t*,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendLaunchCooperativeKernel_t)(
    ze_command_list_handle_t,
    ze_kernel_handle_t,
    const ze_thread_group_dimensions_t*,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendLaunchKernelIndirect_t)(
    ze_command_list_handle_t,
    ze_kernel_handle_t,
    const ze_thread_group_dimensions_t*,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t)(
    ze_command_list_handle_t,
    uint32_t,
    ze_kernel_handle_t*,
    const uint32_t*,
    const ze_thread_group_dimensions_t*,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnCommandListAppendLaunchHostFunction_t)(
    ze_command_list_handle_t,
    ze_host_pfn_t,
    void*,
    ze_event_handle_t,
    uint32_t,
    ze_event_handle_t*
    );

typedef struct _ze_command_list_dditable_t
{
    ze_pfnCommandListCreate_t                                   pfnCreate;
    ze_pfnCommandListCreateImmediate_t                          pfnCreateImmediate;
    ze_pfnCommandListDestroy_t                                  pfnDestroy;
    ze_pfnCommandListClose_t                                    pfnClose;
    ze_pfnCommandListReset_t                                    pfnReset;
    ze_pfnCommandListAppendBarrier_t                            pfnAppendBarrier;
    ze_pfnCommandListAppendMemoryRangesBarrier_t                pfnAppendMemoryRangesBarrier;
    ze_pfnCommandListAppendMemoryCopy_t                         pfnAppendMemoryCopy;
    ze_pfnCommandListAppendMemorySet_t                          pfnAppendMemorySet;
    ze_pfnCommandListAppendMemoryCopyRegion_t                   pfnAppendMemoryCopyRegion;
    ze_pfnCommandListAppendImageCopy_t                          pfnAppendImageCopy;
    ze_pfnCommandListAppendImageCopyRegion_t                    pfnAppendImageCopyRegion;
    ze_pfnCommandListAppendImageCopyToMemory_t                  pfnAppendImageCopyToMemory;
    ze_pfnCommandListAppendImageCopyFromMemory_t                pfnAppendImageCopyFromMemory;
    ze_pfnCommandListAppendMemoryPrefetch_t                     pfnAppendMemoryPrefetch;
    ze_pfnCommandListAppendMemAdvise_t                          pfnAppendMemAdvise;
    ze_pfnCommandListAppendSignalEvent_t                        pfnAppendSignalEvent;
    ze_pfnCommandListAppendWaitOnEvents_t                       pfnAppendWaitOnEvents;
    ze_pfnCommandListAppendEventReset_t                         pfnAppendEventReset;
    ze_pfnCommandListAppendLaunchKernel_t                       pfnAppendLaunchKernel;
    ze_pfnCommandListAppendLaunchCooperativeKernel_t            pfnAppendLaunchCooperativeKernel;
    ze_pfnCommandListAppendLaunchKernelIndirect_t               pfnAppendLaunchKernelIndirect;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t      pfnAppendLaunchMultipleKernelsIndirect;
    ze_pfnCommandListAppendLaunchHostFunction_t                 pfnAppendLaunchHostFunction;
} ze_command_list_dditable_t;

__zedllexport ze_result_t __zecall
zeGetCommandListProcAddrTable(
    ze_api_version_t version,
    ze_command_list_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetCommandListProcAddrTable_t)(
    ze_api_version_t,
    ze_command_list_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnFenceCreate_t)(
    ze_command_queue_handle_t,
    const ze_fence_desc_t*,
    ze_fence_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnFenceDestroy_t)(
    ze_fence_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnFenceHostSynchronize_t)(
    ze_fence_handle_t,
    uint32_t
    );

typedef ze_result_t (__zecall *ze_pfnFenceQueryStatus_t)(
    ze_fence_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnFenceReset_t)(
    ze_fence_handle_t
    );

typedef struct _ze_fence_dditable_t
{
    ze_pfnFenceCreate_t                                         pfnCreate;
    ze_pfnFenceDestroy_t                                        pfnDestroy;
    ze_pfnFenceHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnFenceQueryStatus_t                                    pfnQueryStatus;
    ze_pfnFenceReset_t                                          pfnReset;
} ze_fence_dditable_t;

__zedllexport ze_result_t __zecall
zeGetFenceProcAddrTable(
    ze_api_version_t version,
    ze_fence_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetFenceProcAddrTable_t)(
    ze_api_version_t,
    ze_fence_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventPoolCreate_t)(
    ze_driver_handle_t,
    const ze_event_pool_desc_t*,
    uint32_t,
    ze_device_handle_t*,
    ze_event_pool_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventPoolDestroy_t)(
    ze_event_pool_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnEventPoolGetIpcHandle_t)(
    ze_event_pool_handle_t,
    ze_ipc_event_pool_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventPoolOpenIpcHandle_t)(
    ze_driver_handle_t,
    ze_ipc_event_pool_handle_t,
    ze_event_pool_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventPoolCloseIpcHandle_t)(
    ze_event_pool_handle_t
    );

typedef struct _ze_event_pool_dditable_t
{
    ze_pfnEventPoolCreate_t                                     pfnCreate;
    ze_pfnEventPoolDestroy_t                                    pfnDestroy;
    ze_pfnEventPoolGetIpcHandle_t                               pfnGetIpcHandle;
    ze_pfnEventPoolOpenIpcHandle_t                              pfnOpenIpcHandle;
    ze_pfnEventPoolCloseIpcHandle_t                             pfnCloseIpcHandle;
} ze_event_pool_dditable_t;

__zedllexport ze_result_t __zecall
zeGetEventPoolProcAddrTable(
    ze_api_version_t version,
    ze_event_pool_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetEventPoolProcAddrTable_t)(
    ze_api_version_t,
    ze_event_pool_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventCreate_t)(
    ze_event_pool_handle_t,
    const ze_event_desc_t*,
    ze_event_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnEventDestroy_t)(
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnEventHostSignal_t)(
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnEventHostSynchronize_t)(
    ze_event_handle_t,
    uint32_t
    );

typedef ze_result_t (__zecall *ze_pfnEventQueryStatus_t)(
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnEventReset_t)(
    ze_event_handle_t
    );

typedef struct _ze_event_dditable_t
{
    ze_pfnEventCreate_t                                         pfnCreate;
    ze_pfnEventDestroy_t                                        pfnDestroy;
    ze_pfnEventHostSignal_t                                     pfnHostSignal;
    ze_pfnEventHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnEventQueryStatus_t                                    pfnQueryStatus;
    ze_pfnEventReset_t                                          pfnReset;
} ze_event_dditable_t;

__zedllexport ze_result_t __zecall
zeGetEventProcAddrTable(
    ze_api_version_t version,
    ze_event_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetEventProcAddrTable_t)(
    ze_api_version_t,
    ze_event_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnImageGetProperties_t)(
    ze_device_handle_t,
    const ze_image_desc_t*,
    ze_image_properties_t*
    );

typedef ze_result_t (__zecall *ze_pfnImageCreate_t)(
    ze_device_handle_t,
    const ze_image_desc_t*,
    ze_image_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnImageDestroy_t)(
    ze_image_handle_t
    );

typedef struct _ze_image_dditable_t
{
    ze_pfnImageGetProperties_t                                  pfnGetProperties;
    ze_pfnImageCreate_t                                         pfnCreate;
    ze_pfnImageDestroy_t                                        pfnDestroy;
} ze_image_dditable_t;

__zedllexport ze_result_t __zecall
zeGetImageProcAddrTable(
    ze_api_version_t version,
    ze_image_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetImageProcAddrTable_t)(
    ze_api_version_t,
    ze_image_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnModuleCreate_t)(
    ze_device_handle_t,
    const ze_module_desc_t*,
    ze_module_handle_t*,
    ze_module_build_log_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnModuleDestroy_t)(
    ze_module_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnModuleGetNativeBinary_t)(
    ze_module_handle_t,
    size_t*,
    uint8_t*
    );

typedef ze_result_t (__zecall *ze_pfnModuleGetGlobalPointer_t)(
    ze_module_handle_t,
    const char*,
    void**
    );

typedef ze_result_t (__zecall *ze_pfnModuleGetFunctionPointer_t)(
    ze_module_handle_t,
    const char*,
    void**
    );

typedef struct _ze_module_dditable_t
{
    ze_pfnModuleCreate_t                                        pfnCreate;
    ze_pfnModuleDestroy_t                                       pfnDestroy;
    ze_pfnModuleGetNativeBinary_t                               pfnGetNativeBinary;
    ze_pfnModuleGetGlobalPointer_t                              pfnGetGlobalPointer;
    ze_pfnModuleGetFunctionPointer_t                            pfnGetFunctionPointer;
} ze_module_dditable_t;

__zedllexport ze_result_t __zecall
zeGetModuleProcAddrTable(
    ze_api_version_t version,
    ze_module_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetModuleProcAddrTable_t)(
    ze_api_version_t,
    ze_module_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnModuleBuildLogDestroy_t)(
    ze_module_build_log_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnModuleBuildLogGetString_t)(
    ze_module_build_log_handle_t,
    size_t*,
    char*
    );

typedef struct _ze_module_build_log_dditable_t
{
    ze_pfnModuleBuildLogDestroy_t                               pfnDestroy;
    ze_pfnModuleBuildLogGetString_t                             pfnGetString;
} ze_module_build_log_dditable_t;

__zedllexport ze_result_t __zecall
zeGetModuleBuildLogProcAddrTable(
    ze_api_version_t version,
    ze_module_build_log_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetModuleBuildLogProcAddrTable_t)(
    ze_api_version_t,
    ze_module_build_log_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnKernelCreate_t)(
    ze_module_handle_t,
    const ze_kernel_desc_t*,
    ze_kernel_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnKernelDestroy_t)(
    ze_kernel_handle_t
    );

typedef ze_result_t (__zecall *ze_pfnKernelSetIntermediateCacheConfig_t)(
    ze_kernel_handle_t,
    ze_cache_config_t
    );

typedef ze_result_t (__zecall *ze_pfnKernelSetGroupSize_t)(
    ze_kernel_handle_t,
    uint32_t,
    uint32_t,
    uint32_t
    );

typedef ze_result_t (__zecall *ze_pfnKernelSuggestGroupSize_t)(
    ze_kernel_handle_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t*,
    uint32_t*,
    uint32_t*
    );

typedef ze_result_t (__zecall *ze_pfnKernelSuggestMaxCooperativeGroupCount_t)(
    ze_kernel_handle_t,
    uint32_t*
    );

typedef ze_result_t (__zecall *ze_pfnKernelSetArgumentValue_t)(
    ze_kernel_handle_t,
    uint32_t,
    size_t,
    const void*
    );

typedef ze_result_t (__zecall *ze_pfnKernelSetAttribute_t)(
    ze_kernel_handle_t,
    ze_kernel_set_attribute_t,
    uint32_t
    );

typedef ze_result_t (__zecall *ze_pfnKernelGetProperties_t)(
    ze_kernel_handle_t,
    ze_kernel_properties_t*
    );

typedef struct _ze_kernel_dditable_t
{
    ze_pfnKernelCreate_t                                        pfnCreate;
    ze_pfnKernelDestroy_t                                       pfnDestroy;
    ze_pfnKernelSetIntermediateCacheConfig_t                    pfnSetIntermediateCacheConfig;
    ze_pfnKernelSetGroupSize_t                                  pfnSetGroupSize;
    ze_pfnKernelSuggestGroupSize_t                              pfnSuggestGroupSize;
    ze_pfnKernelSuggestMaxCooperativeGroupCount_t               pfnSuggestMaxCooperativeGroupCount;
    ze_pfnKernelSetArgumentValue_t                              pfnSetArgumentValue;
    ze_pfnKernelSetAttribute_t                                  pfnSetAttribute;
    ze_pfnKernelGetProperties_t                                 pfnGetProperties;
} ze_kernel_dditable_t;

__zedllexport ze_result_t __zecall
zeGetKernelProcAddrTable(
    ze_api_version_t version,
    ze_kernel_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetKernelProcAddrTable_t)(
    ze_api_version_t,
    ze_kernel_dditable_t*
    );

typedef ze_result_t (__zecall *ze_pfnSamplerCreate_t)(
    ze_device_handle_t,
    const ze_sampler_desc_t*,
    ze_sampler_handle_t*
    );

typedef ze_result_t (__zecall *ze_pfnSamplerDestroy_t)(
    ze_sampler_handle_t
    );

typedef struct _ze_sampler_dditable_t
{
    ze_pfnSamplerCreate_t                                       pfnCreate;
    ze_pfnSamplerDestroy_t                                      pfnDestroy;
} ze_sampler_dditable_t;

__zedllexport ze_result_t __zecall
zeGetSamplerProcAddrTable(
    ze_api_version_t version,
    ze_sampler_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *ze_pfnGetSamplerProcAddrTable_t)(
    ze_api_version_t,
    ze_sampler_dditable_t*
    );

typedef struct _ze_dditable_t
{
    ze_global_dditable_t                Global;
    ze_device_dditable_t                Device;
    ze_driver_dditable_t                Driver;
    ze_command_queue_dditable_t         CommandQueue;
    ze_command_list_dditable_t          CommandList;
    ze_fence_dditable_t                 Fence;
    ze_event_pool_dditable_t            EventPool;
    ze_event_dditable_t                 Event;
    ze_image_dditable_t                 Image;
    ze_module_dditable_t                Module;
    ze_module_build_log_dditable_t      ModuleBuildLog;
    ze_kernel_dditable_t                Kernel;
    ze_sampler_dditable_t               Sampler;
} ze_dditable_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_DDI_H
