/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_ddi.h
 * @version v1.9-r1.9.3
 *
 */
#ifndef _ZE_DDI_VER_H
#define _ZE_DDI_VER_H
#if defined(__cplusplus)
#pragma once
#endif
#include "ze_ddi.h"

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// [1.7]
/// @brief Table of RTASBuilderExp functions pointers
typedef struct _ze_rtas_builder_exp_dditable_t_1_7
{
    ze_pfnRTASBuilderCreateExp_t                                pfnCreateExp;
    ze_pfnRTASBuilderGetBuildPropertiesExp_t                    pfnGetBuildPropertiesExp;
    ze_pfnRTASBuilderBuildExp_t                                 pfnBuildExp;
    ze_pfnRTASBuilderDestroyExp_t                               pfnDestroyExp;
} ze_rtas_builder_exp_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.7]
/// @brief Table of RTASParallelOperationExp functions pointers
typedef struct _ze_rtas_parallel_operation_exp_dditable_t_1_7
{
    ze_pfnRTASParallelOperationCreateExp_t                      pfnCreateExp;
    ze_pfnRTASParallelOperationGetPropertiesExp_t               pfnGetPropertiesExp;
    ze_pfnRTASParallelOperationJoinExp_t                        pfnJoinExp;
    ze_pfnRTASParallelOperationDestroyExp_t                     pfnDestroyExp;
} ze_rtas_parallel_operation_exp_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Global functions pointers
typedef struct _ze_global_dditable_t_1_0
{
    ze_pfnInit_t                                                pfnInit;
} ze_global_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.1, 1.6]
/// @brief Table of Driver functions pointers
typedef struct _ze_driver_dditable_t_1_0
{
    ze_pfnDriverGet_t                                           pfnGet;
    ze_pfnDriverGetApiVersion_t                                 pfnGetApiVersion;
    ze_pfnDriverGetProperties_t                                 pfnGetProperties;
    ze_pfnDriverGetIpcProperties_t                              pfnGetIpcProperties;
    ze_pfnDriverGetExtensionProperties_t                        pfnGetExtensionProperties;
} ze_driver_dditable_t_1_0;

typedef struct _ze_driver_dditable_t_1_1
{
    ze_pfnDriverGet_t                                           pfnGet;
    ze_pfnDriverGetApiVersion_t                                 pfnGetApiVersion;
    ze_pfnDriverGetProperties_t                                 pfnGetProperties;
    ze_pfnDriverGetIpcProperties_t                              pfnGetIpcProperties;
    ze_pfnDriverGetExtensionProperties_t                        pfnGetExtensionProperties;
    ze_pfnDriverGetExtensionFunctionAddress_t                   pfnGetExtensionFunctionAddress;
} ze_driver_dditable_t_1_1;

typedef struct _ze_driver_dditable_t_1_6
{
    ze_pfnDriverGet_t                                           pfnGet;
    ze_pfnDriverGetApiVersion_t                                 pfnGetApiVersion;
    ze_pfnDriverGetProperties_t                                 pfnGetProperties;
    ze_pfnDriverGetIpcProperties_t                              pfnGetIpcProperties;
    ze_pfnDriverGetExtensionProperties_t                        pfnGetExtensionProperties;
    ze_pfnDriverGetExtensionFunctionAddress_t                   pfnGetExtensionFunctionAddress;
    ze_pfnDriverGetLastErrorDescription_t                       pfnGetLastErrorDescription;
} ze_driver_dditable_t_1_6;

///////////////////////////////////////////////////////////////////////////////
/// [1.7]
/// @brief Table of DriverExp functions pointers
typedef struct _ze_driver_exp_dditable_t_1_7
{
    ze_pfnDriverRTASFormatCompatibilityCheckExp_t               pfnRTASFormatCompatibilityCheckExp;
} ze_driver_exp_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.1, 1.2, 1.3, 1.7]
/// @brief Table of Device functions pointers
typedef struct _ze_device_dditable_t_1_0
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetModuleProperties_t                           pfnGetModuleProperties;
    ze_pfnDeviceGetCommandQueueGroupProperties_t                pfnGetCommandQueueGroupProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetExternalMemoryProperties_t                   pfnGetExternalMemoryProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceGetStatus_t                                     pfnGetStatus;
} ze_device_dditable_t_1_0;

typedef struct _ze_device_dditable_t_1_1
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetModuleProperties_t                           pfnGetModuleProperties;
    ze_pfnDeviceGetCommandQueueGroupProperties_t                pfnGetCommandQueueGroupProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetExternalMemoryProperties_t                   pfnGetExternalMemoryProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceGetStatus_t                                     pfnGetStatus;
    ze_pfnDeviceGetGlobalTimestamps_t                           pfnGetGlobalTimestamps;
} ze_device_dditable_t_1_1;

typedef struct _ze_device_dditable_t_1_2
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetModuleProperties_t                           pfnGetModuleProperties;
    ze_pfnDeviceGetCommandQueueGroupProperties_t                pfnGetCommandQueueGroupProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetExternalMemoryProperties_t                   pfnGetExternalMemoryProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceGetStatus_t                                     pfnGetStatus;
    ze_pfnDeviceGetGlobalTimestamps_t                           pfnGetGlobalTimestamps;
    ze_pfnDeviceReserveCacheExt_t                               pfnReserveCacheExt;
    ze_pfnDeviceSetCacheAdviceExt_t                             pfnSetCacheAdviceExt;
} ze_device_dditable_t_1_2;

typedef struct _ze_device_dditable_t_1_3
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetModuleProperties_t                           pfnGetModuleProperties;
    ze_pfnDeviceGetCommandQueueGroupProperties_t                pfnGetCommandQueueGroupProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetExternalMemoryProperties_t                   pfnGetExternalMemoryProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceGetStatus_t                                     pfnGetStatus;
    ze_pfnDeviceGetGlobalTimestamps_t                           pfnGetGlobalTimestamps;
    ze_pfnDeviceReserveCacheExt_t                               pfnReserveCacheExt;
    ze_pfnDeviceSetCacheAdviceExt_t                             pfnSetCacheAdviceExt;
    ze_pfnDevicePciGetPropertiesExt_t                           pfnPciGetPropertiesExt;
} ze_device_dditable_t_1_3;

typedef struct _ze_device_dditable_t_1_7
{
    ze_pfnDeviceGet_t                                           pfnGet;
    ze_pfnDeviceGetSubDevices_t                                 pfnGetSubDevices;
    ze_pfnDeviceGetProperties_t                                 pfnGetProperties;
    ze_pfnDeviceGetComputeProperties_t                          pfnGetComputeProperties;
    ze_pfnDeviceGetModuleProperties_t                           pfnGetModuleProperties;
    ze_pfnDeviceGetCommandQueueGroupProperties_t                pfnGetCommandQueueGroupProperties;
    ze_pfnDeviceGetMemoryProperties_t                           pfnGetMemoryProperties;
    ze_pfnDeviceGetMemoryAccessProperties_t                     pfnGetMemoryAccessProperties;
    ze_pfnDeviceGetCacheProperties_t                            pfnGetCacheProperties;
    ze_pfnDeviceGetImageProperties_t                            pfnGetImageProperties;
    ze_pfnDeviceGetExternalMemoryProperties_t                   pfnGetExternalMemoryProperties;
    ze_pfnDeviceGetP2PProperties_t                              pfnGetP2PProperties;
    ze_pfnDeviceCanAccessPeer_t                                 pfnCanAccessPeer;
    ze_pfnDeviceGetStatus_t                                     pfnGetStatus;
    ze_pfnDeviceGetGlobalTimestamps_t                           pfnGetGlobalTimestamps;
    ze_pfnDeviceReserveCacheExt_t                               pfnReserveCacheExt;
    ze_pfnDeviceSetCacheAdviceExt_t                             pfnSetCacheAdviceExt;
    ze_pfnDevicePciGetPropertiesExt_t                           pfnPciGetPropertiesExt;
    ze_pfnDeviceGetRootDevice_t                                 pfnGetRootDevice;
} ze_device_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.4]
/// @brief Table of DeviceExp functions pointers
typedef struct _ze_device_exp_dditable_t_1_4
{
    ze_pfnDeviceGetFabricVertexExp_t                            pfnGetFabricVertexExp;
} ze_device_exp_dditable_t_1_4;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.1]
/// @brief Table of Context functions pointers
typedef struct _ze_context_dditable_t_1_0
{
    ze_pfnContextCreate_t                                       pfnCreate;
    ze_pfnContextDestroy_t                                      pfnDestroy;
    ze_pfnContextGetStatus_t                                    pfnGetStatus;
    ze_pfnContextSystemBarrier_t                                pfnSystemBarrier;
    ze_pfnContextMakeMemoryResident_t                           pfnMakeMemoryResident;
    ze_pfnContextEvictMemory_t                                  pfnEvictMemory;
    ze_pfnContextMakeImageResident_t                            pfnMakeImageResident;
    ze_pfnContextEvictImage_t                                   pfnEvictImage;
} ze_context_dditable_t_1_0;

typedef struct _ze_context_dditable_t_1_1
{
    ze_pfnContextCreate_t                                       pfnCreate;
    ze_pfnContextDestroy_t                                      pfnDestroy;
    ze_pfnContextGetStatus_t                                    pfnGetStatus;
    ze_pfnContextSystemBarrier_t                                pfnSystemBarrier;
    ze_pfnContextMakeMemoryResident_t                           pfnMakeMemoryResident;
    ze_pfnContextEvictMemory_t                                  pfnEvictMemory;
    ze_pfnContextMakeImageResident_t                            pfnMakeImageResident;
    ze_pfnContextEvictImage_t                                   pfnEvictImage;
    ze_pfnContextCreateEx_t                                     pfnCreateEx;
} ze_context_dditable_t_1_1;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.9]
/// @brief Table of CommandQueue functions pointers
typedef struct _ze_command_queue_dditable_t_1_0
{
    ze_pfnCommandQueueCreate_t                                  pfnCreate;
    ze_pfnCommandQueueDestroy_t                                 pfnDestroy;
    ze_pfnCommandQueueExecuteCommandLists_t                     pfnExecuteCommandLists;
    ze_pfnCommandQueueSynchronize_t                             pfnSynchronize;
} ze_command_queue_dditable_t_1_0;

typedef struct _ze_command_queue_dditable_t_1_9
{
    ze_pfnCommandQueueCreate_t                                  pfnCreate;
    ze_pfnCommandQueueDestroy_t                                 pfnDestroy;
    ze_pfnCommandQueueExecuteCommandLists_t                     pfnExecuteCommandLists;
    ze_pfnCommandQueueSynchronize_t                             pfnSynchronize;
    ze_pfnCommandQueueGetOrdinal_t                              pfnGetOrdinal;
    ze_pfnCommandQueueGetIndex_t                                pfnGetIndex;
} ze_command_queue_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.3, 1.6, 1.9]
/// @brief Table of CommandList functions pointers
typedef struct _ze_command_list_dditable_t_1_0
{
    ze_pfnCommandListCreate_t                                   pfnCreate;
    ze_pfnCommandListCreateImmediate_t                          pfnCreateImmediate;
    ze_pfnCommandListDestroy_t                                  pfnDestroy;
    ze_pfnCommandListClose_t                                    pfnClose;
    ze_pfnCommandListReset_t                                    pfnReset;
    ze_pfnCommandListAppendWriteGlobalTimestamp_t               pfnAppendWriteGlobalTimestamp;
    ze_pfnCommandListAppendBarrier_t                            pfnAppendBarrier;
    ze_pfnCommandListAppendMemoryRangesBarrier_t                pfnAppendMemoryRangesBarrier;
    ze_pfnCommandListAppendMemoryCopy_t                         pfnAppendMemoryCopy;
    ze_pfnCommandListAppendMemoryFill_t                         pfnAppendMemoryFill;
    ze_pfnCommandListAppendMemoryCopyRegion_t                   pfnAppendMemoryCopyRegion;
    ze_pfnCommandListAppendMemoryCopyFromContext_t              pfnAppendMemoryCopyFromContext;
    ze_pfnCommandListAppendImageCopy_t                          pfnAppendImageCopy;
    ze_pfnCommandListAppendImageCopyRegion_t                    pfnAppendImageCopyRegion;
    ze_pfnCommandListAppendImageCopyToMemory_t                  pfnAppendImageCopyToMemory;
    ze_pfnCommandListAppendImageCopyFromMemory_t                pfnAppendImageCopyFromMemory;
    ze_pfnCommandListAppendMemoryPrefetch_t                     pfnAppendMemoryPrefetch;
    ze_pfnCommandListAppendMemAdvise_t                          pfnAppendMemAdvise;
    ze_pfnCommandListAppendSignalEvent_t                        pfnAppendSignalEvent;
    ze_pfnCommandListAppendWaitOnEvents_t                       pfnAppendWaitOnEvents;
    ze_pfnCommandListAppendEventReset_t                         pfnAppendEventReset;
    ze_pfnCommandListAppendQueryKernelTimestamps_t              pfnAppendQueryKernelTimestamps;
    ze_pfnCommandListAppendLaunchKernel_t                       pfnAppendLaunchKernel;
    ze_pfnCommandListAppendLaunchCooperativeKernel_t            pfnAppendLaunchCooperativeKernel;
    ze_pfnCommandListAppendLaunchKernelIndirect_t               pfnAppendLaunchKernelIndirect;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t      pfnAppendLaunchMultipleKernelsIndirect;
} ze_command_list_dditable_t_1_0;

typedef struct _ze_command_list_dditable_t_1_3
{
    ze_pfnCommandListCreate_t                                   pfnCreate;
    ze_pfnCommandListCreateImmediate_t                          pfnCreateImmediate;
    ze_pfnCommandListDestroy_t                                  pfnDestroy;
    ze_pfnCommandListClose_t                                    pfnClose;
    ze_pfnCommandListReset_t                                    pfnReset;
    ze_pfnCommandListAppendWriteGlobalTimestamp_t               pfnAppendWriteGlobalTimestamp;
    ze_pfnCommandListAppendBarrier_t                            pfnAppendBarrier;
    ze_pfnCommandListAppendMemoryRangesBarrier_t                pfnAppendMemoryRangesBarrier;
    ze_pfnCommandListAppendMemoryCopy_t                         pfnAppendMemoryCopy;
    ze_pfnCommandListAppendMemoryFill_t                         pfnAppendMemoryFill;
    ze_pfnCommandListAppendMemoryCopyRegion_t                   pfnAppendMemoryCopyRegion;
    ze_pfnCommandListAppendMemoryCopyFromContext_t              pfnAppendMemoryCopyFromContext;
    ze_pfnCommandListAppendImageCopy_t                          pfnAppendImageCopy;
    ze_pfnCommandListAppendImageCopyRegion_t                    pfnAppendImageCopyRegion;
    ze_pfnCommandListAppendImageCopyToMemory_t                  pfnAppendImageCopyToMemory;
    ze_pfnCommandListAppendImageCopyFromMemory_t                pfnAppendImageCopyFromMemory;
    ze_pfnCommandListAppendMemoryPrefetch_t                     pfnAppendMemoryPrefetch;
    ze_pfnCommandListAppendMemAdvise_t                          pfnAppendMemAdvise;
    ze_pfnCommandListAppendSignalEvent_t                        pfnAppendSignalEvent;
    ze_pfnCommandListAppendWaitOnEvents_t                       pfnAppendWaitOnEvents;
    ze_pfnCommandListAppendEventReset_t                         pfnAppendEventReset;
    ze_pfnCommandListAppendQueryKernelTimestamps_t              pfnAppendQueryKernelTimestamps;
    ze_pfnCommandListAppendLaunchKernel_t                       pfnAppendLaunchKernel;
    ze_pfnCommandListAppendLaunchCooperativeKernel_t            pfnAppendLaunchCooperativeKernel;
    ze_pfnCommandListAppendLaunchKernelIndirect_t               pfnAppendLaunchKernelIndirect;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t      pfnAppendLaunchMultipleKernelsIndirect;
    ze_pfnCommandListAppendImageCopyToMemoryExt_t               pfnAppendImageCopyToMemoryExt;
    ze_pfnCommandListAppendImageCopyFromMemoryExt_t             pfnAppendImageCopyFromMemoryExt;
} ze_command_list_dditable_t_1_3;

typedef struct _ze_command_list_dditable_t_1_6
{
    ze_pfnCommandListCreate_t                                   pfnCreate;
    ze_pfnCommandListCreateImmediate_t                          pfnCreateImmediate;
    ze_pfnCommandListDestroy_t                                  pfnDestroy;
    ze_pfnCommandListClose_t                                    pfnClose;
    ze_pfnCommandListReset_t                                    pfnReset;
    ze_pfnCommandListAppendWriteGlobalTimestamp_t               pfnAppendWriteGlobalTimestamp;
    ze_pfnCommandListAppendBarrier_t                            pfnAppendBarrier;
    ze_pfnCommandListAppendMemoryRangesBarrier_t                pfnAppendMemoryRangesBarrier;
    ze_pfnCommandListAppendMemoryCopy_t                         pfnAppendMemoryCopy;
    ze_pfnCommandListAppendMemoryFill_t                         pfnAppendMemoryFill;
    ze_pfnCommandListAppendMemoryCopyRegion_t                   pfnAppendMemoryCopyRegion;
    ze_pfnCommandListAppendMemoryCopyFromContext_t              pfnAppendMemoryCopyFromContext;
    ze_pfnCommandListAppendImageCopy_t                          pfnAppendImageCopy;
    ze_pfnCommandListAppendImageCopyRegion_t                    pfnAppendImageCopyRegion;
    ze_pfnCommandListAppendImageCopyToMemory_t                  pfnAppendImageCopyToMemory;
    ze_pfnCommandListAppendImageCopyFromMemory_t                pfnAppendImageCopyFromMemory;
    ze_pfnCommandListAppendMemoryPrefetch_t                     pfnAppendMemoryPrefetch;
    ze_pfnCommandListAppendMemAdvise_t                          pfnAppendMemAdvise;
    ze_pfnCommandListAppendSignalEvent_t                        pfnAppendSignalEvent;
    ze_pfnCommandListAppendWaitOnEvents_t                       pfnAppendWaitOnEvents;
    ze_pfnCommandListAppendEventReset_t                         pfnAppendEventReset;
    ze_pfnCommandListAppendQueryKernelTimestamps_t              pfnAppendQueryKernelTimestamps;
    ze_pfnCommandListAppendLaunchKernel_t                       pfnAppendLaunchKernel;
    ze_pfnCommandListAppendLaunchCooperativeKernel_t            pfnAppendLaunchCooperativeKernel;
    ze_pfnCommandListAppendLaunchKernelIndirect_t               pfnAppendLaunchKernelIndirect;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t      pfnAppendLaunchMultipleKernelsIndirect;
    ze_pfnCommandListAppendImageCopyToMemoryExt_t               pfnAppendImageCopyToMemoryExt;
    ze_pfnCommandListAppendImageCopyFromMemoryExt_t             pfnAppendImageCopyFromMemoryExt;
    ze_pfnCommandListHostSynchronize_t                          pfnHostSynchronize;
} ze_command_list_dditable_t_1_6;

typedef struct _ze_command_list_dditable_t_1_9
{
    ze_pfnCommandListCreate_t                                   pfnCreate;
    ze_pfnCommandListCreateImmediate_t                          pfnCreateImmediate;
    ze_pfnCommandListDestroy_t                                  pfnDestroy;
    ze_pfnCommandListClose_t                                    pfnClose;
    ze_pfnCommandListReset_t                                    pfnReset;
    ze_pfnCommandListAppendWriteGlobalTimestamp_t               pfnAppendWriteGlobalTimestamp;
    ze_pfnCommandListAppendBarrier_t                            pfnAppendBarrier;
    ze_pfnCommandListAppendMemoryRangesBarrier_t                pfnAppendMemoryRangesBarrier;
    ze_pfnCommandListAppendMemoryCopy_t                         pfnAppendMemoryCopy;
    ze_pfnCommandListAppendMemoryFill_t                         pfnAppendMemoryFill;
    ze_pfnCommandListAppendMemoryCopyRegion_t                   pfnAppendMemoryCopyRegion;
    ze_pfnCommandListAppendMemoryCopyFromContext_t              pfnAppendMemoryCopyFromContext;
    ze_pfnCommandListAppendImageCopy_t                          pfnAppendImageCopy;
    ze_pfnCommandListAppendImageCopyRegion_t                    pfnAppendImageCopyRegion;
    ze_pfnCommandListAppendImageCopyToMemory_t                  pfnAppendImageCopyToMemory;
    ze_pfnCommandListAppendImageCopyFromMemory_t                pfnAppendImageCopyFromMemory;
    ze_pfnCommandListAppendMemoryPrefetch_t                     pfnAppendMemoryPrefetch;
    ze_pfnCommandListAppendMemAdvise_t                          pfnAppendMemAdvise;
    ze_pfnCommandListAppendSignalEvent_t                        pfnAppendSignalEvent;
    ze_pfnCommandListAppendWaitOnEvents_t                       pfnAppendWaitOnEvents;
    ze_pfnCommandListAppendEventReset_t                         pfnAppendEventReset;
    ze_pfnCommandListAppendQueryKernelTimestamps_t              pfnAppendQueryKernelTimestamps;
    ze_pfnCommandListAppendLaunchKernel_t                       pfnAppendLaunchKernel;
    ze_pfnCommandListAppendLaunchCooperativeKernel_t            pfnAppendLaunchCooperativeKernel;
    ze_pfnCommandListAppendLaunchKernelIndirect_t               pfnAppendLaunchKernelIndirect;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirect_t      pfnAppendLaunchMultipleKernelsIndirect;
    ze_pfnCommandListAppendImageCopyToMemoryExt_t               pfnAppendImageCopyToMemoryExt;
    ze_pfnCommandListAppendImageCopyFromMemoryExt_t             pfnAppendImageCopyFromMemoryExt;
    ze_pfnCommandListHostSynchronize_t                          pfnHostSynchronize;
    ze_pfnCommandListGetDeviceHandle_t                          pfnGetDeviceHandle;
    ze_pfnCommandListGetContextHandle_t                         pfnGetContextHandle;
    ze_pfnCommandListGetOrdinal_t                               pfnGetOrdinal;
    ze_pfnCommandListImmediateGetIndex_t                        pfnImmediateGetIndex;
    ze_pfnCommandListIsImmediate_t                              pfnIsImmediate;
} ze_command_list_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of CommandListExp functions pointers
typedef struct _ze_command_list_exp_dditable_t_1_9
{
    ze_pfnCommandListCreateCloneExp_t                           pfnCreateCloneExp;
    ze_pfnCommandListImmediateAppendCommandListsExp_t           pfnImmediateAppendCommandListsExp;
    ze_pfnCommandListGetNextCommandIdExp_t                      pfnGetNextCommandIdExp;
    ze_pfnCommandListUpdateMutableCommandsExp_t                 pfnUpdateMutableCommandsExp;
    ze_pfnCommandListUpdateMutableCommandSignalEventExp_t       pfnUpdateMutableCommandSignalEventExp;
    ze_pfnCommandListUpdateMutableCommandWaitEventsExp_t        pfnUpdateMutableCommandWaitEventsExp;
} ze_command_list_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.3, 1.5]
/// @brief Table of Image functions pointers
typedef struct _ze_image_dditable_t_1_0
{
    ze_pfnImageGetProperties_t                                  pfnGetProperties;
    ze_pfnImageCreate_t                                         pfnCreate;
    ze_pfnImageDestroy_t                                        pfnDestroy;
} ze_image_dditable_t_1_0;

typedef struct _ze_image_dditable_t_1_3
{
    ze_pfnImageGetProperties_t                                  pfnGetProperties;
    ze_pfnImageCreate_t                                         pfnCreate;
    ze_pfnImageDestroy_t                                        pfnDestroy;
    ze_pfnImageGetAllocPropertiesExt_t                          pfnGetAllocPropertiesExt;
} ze_image_dditable_t_1_3;

typedef struct _ze_image_dditable_t_1_5
{
    ze_pfnImageGetProperties_t                                  pfnGetProperties;
    ze_pfnImageCreate_t                                         pfnCreate;
    ze_pfnImageDestroy_t                                        pfnDestroy;
    ze_pfnImageGetAllocPropertiesExt_t                          pfnGetAllocPropertiesExt;
    ze_pfnImageViewCreateExt_t                                  pfnViewCreateExt;
} ze_image_dditable_t_1_5;

///////////////////////////////////////////////////////////////////////////////
/// [1.2, 1.9]
/// @brief Table of ImageExp functions pointers
typedef struct _ze_image_exp_dditable_t_1_2
{
    ze_pfnImageGetMemoryPropertiesExp_t                         pfnGetMemoryPropertiesExp;
    ze_pfnImageViewCreateExp_t                                  pfnViewCreateExp;
} ze_image_exp_dditable_t_1_2;

typedef struct _ze_image_exp_dditable_t_1_9
{
    ze_pfnImageGetMemoryPropertiesExp_t                         pfnGetMemoryPropertiesExp;
    ze_pfnImageViewCreateExp_t                                  pfnViewCreateExp;
    ze_pfnImageGetDeviceOffsetExp_t                             pfnGetDeviceOffsetExp;
} ze_image_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.3, 1.6, 1.9]
/// @brief Table of Mem functions pointers
typedef struct _ze_mem_dditable_t_1_0
{
    ze_pfnMemAllocShared_t                                      pfnAllocShared;
    ze_pfnMemAllocDevice_t                                      pfnAllocDevice;
    ze_pfnMemAllocHost_t                                        pfnAllocHost;
    ze_pfnMemFree_t                                             pfnFree;
    ze_pfnMemGetAllocProperties_t                               pfnGetAllocProperties;
    ze_pfnMemGetAddressRange_t                                  pfnGetAddressRange;
    ze_pfnMemGetIpcHandle_t                                     pfnGetIpcHandle;
    ze_pfnMemOpenIpcHandle_t                                    pfnOpenIpcHandle;
    ze_pfnMemCloseIpcHandle_t                                   pfnCloseIpcHandle;
} ze_mem_dditable_t_1_0;

typedef struct _ze_mem_dditable_t_1_3
{
    ze_pfnMemAllocShared_t                                      pfnAllocShared;
    ze_pfnMemAllocDevice_t                                      pfnAllocDevice;
    ze_pfnMemAllocHost_t                                        pfnAllocHost;
    ze_pfnMemFree_t                                             pfnFree;
    ze_pfnMemGetAllocProperties_t                               pfnGetAllocProperties;
    ze_pfnMemGetAddressRange_t                                  pfnGetAddressRange;
    ze_pfnMemGetIpcHandle_t                                     pfnGetIpcHandle;
    ze_pfnMemOpenIpcHandle_t                                    pfnOpenIpcHandle;
    ze_pfnMemCloseIpcHandle_t                                   pfnCloseIpcHandle;
    ze_pfnMemFreeExt_t                                          pfnFreeExt;
} ze_mem_dditable_t_1_3;

typedef struct _ze_mem_dditable_t_1_6
{
    ze_pfnMemAllocShared_t                                      pfnAllocShared;
    ze_pfnMemAllocDevice_t                                      pfnAllocDevice;
    ze_pfnMemAllocHost_t                                        pfnAllocHost;
    ze_pfnMemFree_t                                             pfnFree;
    ze_pfnMemGetAllocProperties_t                               pfnGetAllocProperties;
    ze_pfnMemGetAddressRange_t                                  pfnGetAddressRange;
    ze_pfnMemGetIpcHandle_t                                     pfnGetIpcHandle;
    ze_pfnMemOpenIpcHandle_t                                    pfnOpenIpcHandle;
    ze_pfnMemCloseIpcHandle_t                                   pfnCloseIpcHandle;
    ze_pfnMemFreeExt_t                                          pfnFreeExt;
    ze_pfnMemPutIpcHandle_t                                     pfnPutIpcHandle;
} ze_mem_dditable_t_1_6;

typedef struct _ze_mem_dditable_t_1_9
{
    ze_pfnMemAllocShared_t                                      pfnAllocShared;
    ze_pfnMemAllocDevice_t                                      pfnAllocDevice;
    ze_pfnMemAllocHost_t                                        pfnAllocHost;
    ze_pfnMemFree_t                                             pfnFree;
    ze_pfnMemGetAllocProperties_t                               pfnGetAllocProperties;
    ze_pfnMemGetAddressRange_t                                  pfnGetAddressRange;
    ze_pfnMemGetIpcHandle_t                                     pfnGetIpcHandle;
    ze_pfnMemOpenIpcHandle_t                                    pfnOpenIpcHandle;
    ze_pfnMemCloseIpcHandle_t                                   pfnCloseIpcHandle;
    ze_pfnMemFreeExt_t                                          pfnFreeExt;
    ze_pfnMemPutIpcHandle_t                                     pfnPutIpcHandle;
    ze_pfnMemGetPitchFor2dImage_t                               pfnGetPitchFor2dImage;
} ze_mem_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.6, 1.7]
/// @brief Table of MemExp functions pointers
typedef struct _ze_mem_exp_dditable_t_1_6
{
    ze_pfnMemGetIpcHandleFromFileDescriptorExp_t                pfnGetIpcHandleFromFileDescriptorExp;
    ze_pfnMemGetFileDescriptorFromIpcHandleExp_t                pfnGetFileDescriptorFromIpcHandleExp;
} ze_mem_exp_dditable_t_1_6;

typedef struct _ze_mem_exp_dditable_t_1_7
{
    ze_pfnMemGetIpcHandleFromFileDescriptorExp_t                pfnGetIpcHandleFromFileDescriptorExp;
    ze_pfnMemGetFileDescriptorFromIpcHandleExp_t                pfnGetFileDescriptorFromIpcHandleExp;
    ze_pfnMemSetAtomicAccessAttributeExp_t                      pfnSetAtomicAccessAttributeExp;
    ze_pfnMemGetAtomicAccessAttributeExp_t                      pfnGetAtomicAccessAttributeExp;
} ze_mem_exp_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Fence functions pointers
typedef struct _ze_fence_dditable_t_1_0
{
    ze_pfnFenceCreate_t                                         pfnCreate;
    ze_pfnFenceDestroy_t                                        pfnDestroy;
    ze_pfnFenceHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnFenceQueryStatus_t                                    pfnQueryStatus;
    ze_pfnFenceReset_t                                          pfnReset;
} ze_fence_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.6, 1.9]
/// @brief Table of EventPool functions pointers
typedef struct _ze_event_pool_dditable_t_1_0
{
    ze_pfnEventPoolCreate_t                                     pfnCreate;
    ze_pfnEventPoolDestroy_t                                    pfnDestroy;
    ze_pfnEventPoolGetIpcHandle_t                               pfnGetIpcHandle;
    ze_pfnEventPoolOpenIpcHandle_t                              pfnOpenIpcHandle;
    ze_pfnEventPoolCloseIpcHandle_t                             pfnCloseIpcHandle;
} ze_event_pool_dditable_t_1_0;

typedef struct _ze_event_pool_dditable_t_1_6
{
    ze_pfnEventPoolCreate_t                                     pfnCreate;
    ze_pfnEventPoolDestroy_t                                    pfnDestroy;
    ze_pfnEventPoolGetIpcHandle_t                               pfnGetIpcHandle;
    ze_pfnEventPoolOpenIpcHandle_t                              pfnOpenIpcHandle;
    ze_pfnEventPoolCloseIpcHandle_t                             pfnCloseIpcHandle;
    ze_pfnEventPoolPutIpcHandle_t                               pfnPutIpcHandle;
} ze_event_pool_dditable_t_1_6;

typedef struct _ze_event_pool_dditable_t_1_9
{
    ze_pfnEventPoolCreate_t                                     pfnCreate;
    ze_pfnEventPoolDestroy_t                                    pfnDestroy;
    ze_pfnEventPoolGetIpcHandle_t                               pfnGetIpcHandle;
    ze_pfnEventPoolOpenIpcHandle_t                              pfnOpenIpcHandle;
    ze_pfnEventPoolCloseIpcHandle_t                             pfnCloseIpcHandle;
    ze_pfnEventPoolPutIpcHandle_t                               pfnPutIpcHandle;
    ze_pfnEventPoolGetContextHandle_t                           pfnGetContextHandle;
    ze_pfnEventPoolGetFlags_t                                   pfnGetFlags;
} ze_event_pool_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.6, 1.9]
/// @brief Table of Event functions pointers
typedef struct _ze_event_dditable_t_1_0
{
    ze_pfnEventCreate_t                                         pfnCreate;
    ze_pfnEventDestroy_t                                        pfnDestroy;
    ze_pfnEventHostSignal_t                                     pfnHostSignal;
    ze_pfnEventHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnEventQueryStatus_t                                    pfnQueryStatus;
    ze_pfnEventHostReset_t                                      pfnHostReset;
    ze_pfnEventQueryKernelTimestamp_t                           pfnQueryKernelTimestamp;
} ze_event_dditable_t_1_0;

typedef struct _ze_event_dditable_t_1_6
{
    ze_pfnEventCreate_t                                         pfnCreate;
    ze_pfnEventDestroy_t                                        pfnDestroy;
    ze_pfnEventHostSignal_t                                     pfnHostSignal;
    ze_pfnEventHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnEventQueryStatus_t                                    pfnQueryStatus;
    ze_pfnEventHostReset_t                                      pfnHostReset;
    ze_pfnEventQueryKernelTimestamp_t                           pfnQueryKernelTimestamp;
    ze_pfnEventQueryKernelTimestampsExt_t                       pfnQueryKernelTimestampsExt;
} ze_event_dditable_t_1_6;

typedef struct _ze_event_dditable_t_1_9
{
    ze_pfnEventCreate_t                                         pfnCreate;
    ze_pfnEventDestroy_t                                        pfnDestroy;
    ze_pfnEventHostSignal_t                                     pfnHostSignal;
    ze_pfnEventHostSynchronize_t                                pfnHostSynchronize;
    ze_pfnEventQueryStatus_t                                    pfnQueryStatus;
    ze_pfnEventHostReset_t                                      pfnHostReset;
    ze_pfnEventQueryKernelTimestamp_t                           pfnQueryKernelTimestamp;
    ze_pfnEventQueryKernelTimestampsExt_t                       pfnQueryKernelTimestampsExt;
    ze_pfnEventGetEventPool_t                                   pfnGetEventPool;
    ze_pfnEventGetSignalScope_t                                 pfnGetSignalScope;
    ze_pfnEventGetWaitScope_t                                   pfnGetWaitScope;
} ze_event_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.2]
/// @brief Table of EventExp functions pointers
typedef struct _ze_event_exp_dditable_t_1_2
{
    ze_pfnEventQueryTimestampsExp_t                             pfnQueryTimestampsExp;
} ze_event_exp_dditable_t_1_2;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.3]
/// @brief Table of Module functions pointers
typedef struct _ze_module_dditable_t_1_0
{
    ze_pfnModuleCreate_t                                        pfnCreate;
    ze_pfnModuleDestroy_t                                       pfnDestroy;
    ze_pfnModuleDynamicLink_t                                   pfnDynamicLink;
    ze_pfnModuleGetNativeBinary_t                               pfnGetNativeBinary;
    ze_pfnModuleGetGlobalPointer_t                              pfnGetGlobalPointer;
    ze_pfnModuleGetKernelNames_t                                pfnGetKernelNames;
    ze_pfnModuleGetProperties_t                                 pfnGetProperties;
    ze_pfnModuleGetFunctionPointer_t                            pfnGetFunctionPointer;
} ze_module_dditable_t_1_0;

typedef struct _ze_module_dditable_t_1_3
{
    ze_pfnModuleCreate_t                                        pfnCreate;
    ze_pfnModuleDestroy_t                                       pfnDestroy;
    ze_pfnModuleDynamicLink_t                                   pfnDynamicLink;
    ze_pfnModuleGetNativeBinary_t                               pfnGetNativeBinary;
    ze_pfnModuleGetGlobalPointer_t                              pfnGetGlobalPointer;
    ze_pfnModuleGetKernelNames_t                                pfnGetKernelNames;
    ze_pfnModuleGetProperties_t                                 pfnGetProperties;
    ze_pfnModuleGetFunctionPointer_t                            pfnGetFunctionPointer;
    ze_pfnModuleInspectLinkageExt_t                             pfnInspectLinkageExt;
} ze_module_dditable_t_1_3;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of ModuleBuildLog functions pointers
typedef struct _ze_module_build_log_dditable_t_1_0
{
    ze_pfnModuleBuildLogDestroy_t                               pfnDestroy;
    ze_pfnModuleBuildLogGetString_t                             pfnGetString;
} ze_module_build_log_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Kernel functions pointers
typedef struct _ze_kernel_dditable_t_1_0
{
    ze_pfnKernelCreate_t                                        pfnCreate;
    ze_pfnKernelDestroy_t                                       pfnDestroy;
    ze_pfnKernelSetCacheConfig_t                                pfnSetCacheConfig;
    ze_pfnKernelSetGroupSize_t                                  pfnSetGroupSize;
    ze_pfnKernelSuggestGroupSize_t                              pfnSuggestGroupSize;
    ze_pfnKernelSuggestMaxCooperativeGroupCount_t               pfnSuggestMaxCooperativeGroupCount;
    ze_pfnKernelSetArgumentValue_t                              pfnSetArgumentValue;
    ze_pfnKernelSetIndirectAccess_t                             pfnSetIndirectAccess;
    ze_pfnKernelGetIndirectAccess_t                             pfnGetIndirectAccess;
    ze_pfnKernelGetSourceAttributes_t                           pfnGetSourceAttributes;
    ze_pfnKernelGetProperties_t                                 pfnGetProperties;
    ze_pfnKernelGetName_t                                       pfnGetName;
} ze_kernel_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.1, 1.2]
/// @brief Table of KernelExp functions pointers
typedef struct _ze_kernel_exp_dditable_t_1_1
{
    ze_pfnKernelSetGlobalOffsetExp_t                            pfnSetGlobalOffsetExp;
} ze_kernel_exp_dditable_t_1_1;

typedef struct _ze_kernel_exp_dditable_t_1_2
{
    ze_pfnKernelSetGlobalOffsetExp_t                            pfnSetGlobalOffsetExp;
    ze_pfnKernelSchedulingHintExp_t                             pfnSchedulingHintExp;
} ze_kernel_exp_dditable_t_1_2;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Sampler functions pointers
typedef struct _ze_sampler_dditable_t_1_0
{
    ze_pfnSamplerCreate_t                                       pfnCreate;
    ze_pfnSamplerDestroy_t                                      pfnDestroy;
} ze_sampler_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of PhysicalMem functions pointers
typedef struct _ze_physical_mem_dditable_t_1_0
{
    ze_pfnPhysicalMemCreate_t                                   pfnCreate;
    ze_pfnPhysicalMemDestroy_t                                  pfnDestroy;
} ze_physical_mem_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of VirtualMem functions pointers
typedef struct _ze_virtual_mem_dditable_t_1_0
{
    ze_pfnVirtualMemReserve_t                                   pfnReserve;
    ze_pfnVirtualMemFree_t                                      pfnFree;
    ze_pfnVirtualMemQueryPageSize_t                             pfnQueryPageSize;
    ze_pfnVirtualMemMap_t                                       pfnMap;
    ze_pfnVirtualMemUnmap_t                                     pfnUnmap;
    ze_pfnVirtualMemSetAccessAttribute_t                        pfnSetAccessAttribute;
    ze_pfnVirtualMemGetAccessAttribute_t                        pfnGetAccessAttribute;
} ze_virtual_mem_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.4]
/// @brief Table of FabricVertexExp functions pointers
typedef struct _ze_fabric_vertex_exp_dditable_t_1_4
{
    ze_pfnFabricVertexGetExp_t                                  pfnGetExp;
    ze_pfnFabricVertexGetSubVerticesExp_t                       pfnGetSubVerticesExp;
    ze_pfnFabricVertexGetPropertiesExp_t                        pfnGetPropertiesExp;
    ze_pfnFabricVertexGetDeviceExp_t                            pfnGetDeviceExp;
} ze_fabric_vertex_exp_dditable_t_1_4;

///////////////////////////////////////////////////////////////////////////////
/// [1.4]
/// @brief Table of FabricEdgeExp functions pointers
typedef struct _ze_fabric_edge_exp_dditable_t_1_4
{
    ze_pfnFabricEdgeGetExp_t                                    pfnGetExp;
    ze_pfnFabricEdgeGetVerticesExp_t                            pfnGetVerticesExp;
    ze_pfnFabricEdgeGetPropertiesExp_t                          pfnGetPropertiesExp;
} ze_fabric_edge_exp_dditable_t_1_4;


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_DDI_VER_H
