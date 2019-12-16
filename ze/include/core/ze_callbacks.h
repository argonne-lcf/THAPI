/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_callbacks.h
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core
 * @endcond
 *
 */
#ifndef _ZE_CALLBACKS_H
#define _ZE_CALLBACKS_H
#if defined(__cplusplus)
#pragma once
#endif
#include "ze_api.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _ze_init_params_t
{
    ze_init_flag_t* pflags;
} ze_init_params_t;

typedef void (__zecall *ze_pfnInitCb_t)(
    ze_init_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_global_callbacks_t
{
    ze_pfnInitCb_t                                                  pfnInitCb;
} ze_global_callbacks_t;

typedef struct _ze_device_get_params_t
{
    ze_driver_handle_t* phDriver;
    uint32_t** ppCount;
    ze_device_handle_t** pphDevices;
} ze_device_get_params_t;

typedef void (__zecall *ze_pfnDeviceGetCb_t)(
    ze_device_get_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_sub_devices_params_t
{
    ze_device_handle_t* phDevice;
    uint32_t** ppCount;
    ze_device_handle_t** pphSubdevices;
} ze_device_get_sub_devices_params_t;

typedef void (__zecall *ze_pfnDeviceGetSubDevicesCb_t)(
    ze_device_get_sub_devices_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_properties_t** ppDeviceProperties;
} ze_device_get_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetPropertiesCb_t)(
    ze_device_get_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_compute_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_compute_properties_t** ppComputeProperties;
} ze_device_get_compute_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetComputePropertiesCb_t)(
    ze_device_get_compute_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_memory_properties_params_t
{
    ze_device_handle_t* phDevice;
    uint32_t** ppCount;
    ze_device_memory_properties_t** ppMemProperties;
} ze_device_get_memory_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetMemoryPropertiesCb_t)(
    ze_device_get_memory_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_memory_access_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_memory_access_properties_t** ppMemAccessProperties;
} ze_device_get_memory_access_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetMemoryAccessPropertiesCb_t)(
    ze_device_get_memory_access_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_cache_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_cache_properties_t** ppCacheProperties;
} ze_device_get_cache_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetCachePropertiesCb_t)(
    ze_device_get_cache_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_image_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_image_properties_t** ppImageProperties;
} ze_device_get_image_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetImagePropertiesCb_t)(
    ze_device_get_image_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_get_p2_p_properties_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_handle_t* phPeerDevice;
    ze_device_p2p_properties_t** ppP2PProperties;
} ze_device_get_p2_p_properties_params_t;

typedef void (__zecall *ze_pfnDeviceGetP2PPropertiesCb_t)(
    ze_device_get_p2_p_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_can_access_peer_params_t
{
    ze_device_handle_t* phDevice;
    ze_device_handle_t* phPeerDevice;
    ze_bool_t** pvalue;
} ze_device_can_access_peer_params_t;

typedef void (__zecall *ze_pfnDeviceCanAccessPeerCb_t)(
    ze_device_can_access_peer_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_set_last_level_cache_config_params_t
{
    ze_device_handle_t* phDevice;
    ze_cache_config_t* pCacheConfig;
} ze_device_set_last_level_cache_config_params_t;

typedef void (__zecall *ze_pfnDeviceSetLastLevelCacheConfigCb_t)(
    ze_device_set_last_level_cache_config_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_system_barrier_params_t
{
    ze_device_handle_t* phDevice;
} ze_device_system_barrier_params_t;

typedef void (__zecall *ze_pfnDeviceSystemBarrierCb_t)(
    ze_device_system_barrier_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

#if ZE_ENABLE_OCL_INTEROP
typedef struct _ze_device_register_cl_memory_params_t
{
    ze_device_handle_t* phDevice;
    cl_context* pcontext;
    cl_mem* pmem;
    void*** pptr;
} ze_device_register_cl_memory_params_t;
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef void (__zecall *ze_pfnDeviceRegisterCLMemoryCb_t)(
    ze_device_register_cl_memory_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef struct _ze_device_register_cl_program_params_t
{
    ze_device_handle_t* phDevice;
    cl_context* pcontext;
    cl_program* pprogram;
    ze_module_handle_t** pphModule;
} ze_device_register_cl_program_params_t;
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef void (__zecall *ze_pfnDeviceRegisterCLProgramCb_t)(
    ze_device_register_cl_program_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef struct _ze_device_register_cl_command_queue_params_t
{
    ze_device_handle_t* phDevice;
    cl_context* pcontext;
    cl_command_queue* pcommand_queue;
    ze_command_queue_handle_t** pphCommandQueue;
} ze_device_register_cl_command_queue_params_t;
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
typedef void (__zecall *ze_pfnDeviceRegisterCLCommandQueueCb_t)(
    ze_device_register_cl_command_queue_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );
#endif // ZE_ENABLE_OCL_INTEROP

typedef struct _ze_device_make_memory_resident_params_t
{
    ze_device_handle_t* phDevice;
    void** pptr;
    size_t* psize;
} ze_device_make_memory_resident_params_t;

typedef void (__zecall *ze_pfnDeviceMakeMemoryResidentCb_t)(
    ze_device_make_memory_resident_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_evict_memory_params_t
{
    ze_device_handle_t* phDevice;
    void** pptr;
    size_t* psize;
} ze_device_evict_memory_params_t;

typedef void (__zecall *ze_pfnDeviceEvictMemoryCb_t)(
    ze_device_evict_memory_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_make_image_resident_params_t
{
    ze_device_handle_t* phDevice;
    ze_image_handle_t* phImage;
} ze_device_make_image_resident_params_t;

typedef void (__zecall *ze_pfnDeviceMakeImageResidentCb_t)(
    ze_device_make_image_resident_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_evict_image_params_t
{
    ze_device_handle_t* phDevice;
    ze_image_handle_t* phImage;
} ze_device_evict_image_params_t;

typedef void (__zecall *ze_pfnDeviceEvictImageCb_t)(
    ze_device_evict_image_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_device_callbacks_t
{
    ze_pfnDeviceGetCb_t                                             pfnGetCb;
    ze_pfnDeviceGetSubDevicesCb_t                                   pfnGetSubDevicesCb;
    ze_pfnDeviceGetPropertiesCb_t                                   pfnGetPropertiesCb;
    ze_pfnDeviceGetComputePropertiesCb_t                            pfnGetComputePropertiesCb;
    ze_pfnDeviceGetMemoryPropertiesCb_t                             pfnGetMemoryPropertiesCb;
    ze_pfnDeviceGetMemoryAccessPropertiesCb_t                       pfnGetMemoryAccessPropertiesCb;
    ze_pfnDeviceGetCachePropertiesCb_t                              pfnGetCachePropertiesCb;
    ze_pfnDeviceGetImagePropertiesCb_t                              pfnGetImagePropertiesCb;
    ze_pfnDeviceGetP2PPropertiesCb_t                                pfnGetP2PPropertiesCb;
    ze_pfnDeviceCanAccessPeerCb_t                                   pfnCanAccessPeerCb;
    ze_pfnDeviceSetLastLevelCacheConfigCb_t                         pfnSetLastLevelCacheConfigCb;
    ze_pfnDeviceSystemBarrierCb_t                                   pfnSystemBarrierCb;
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLMemoryCb_t                                pfnRegisterCLMemoryCb;
#endif // ZE_ENABLE_OCL_INTEROP
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLProgramCb_t                               pfnRegisterCLProgramCb;
#endif // ZE_ENABLE_OCL_INTEROP
#if ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceRegisterCLCommandQueueCb_t                          pfnRegisterCLCommandQueueCb;
#endif // ZE_ENABLE_OCL_INTEROP
    ze_pfnDeviceMakeMemoryResidentCb_t                              pfnMakeMemoryResidentCb;
    ze_pfnDeviceEvictMemoryCb_t                                     pfnEvictMemoryCb;
    ze_pfnDeviceMakeImageResidentCb_t                               pfnMakeImageResidentCb;
    ze_pfnDeviceEvictImageCb_t                                      pfnEvictImageCb;
} ze_device_callbacks_t;

typedef struct _ze_driver_get_params_t
{
    uint32_t** ppCount;
    ze_driver_handle_t** pphDrivers;
} ze_driver_get_params_t;

typedef void (__zecall *ze_pfnDriverGetCb_t)(
    ze_driver_get_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_driver_version_params_t
{
    ze_driver_handle_t* phDriver;
    uint32_t** pversion;
} ze_driver_get_driver_version_params_t;

typedef void (__zecall *ze_pfnDriverGetDriverVersionCb_t)(
    ze_driver_get_driver_version_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_api_version_params_t
{
    ze_driver_handle_t* phDrivers;
    ze_api_version_t** pversion;
} ze_driver_get_api_version_params_t;

typedef void (__zecall *ze_pfnDriverGetApiVersionCb_t)(
    ze_driver_get_api_version_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_ipc_properties_params_t
{
    ze_driver_handle_t* phDriver;
    ze_driver_ipc_properties_t** ppIPCProperties;
} ze_driver_get_ipc_properties_params_t;

typedef void (__zecall *ze_pfnDriverGetIPCPropertiesCb_t)(
    ze_driver_get_ipc_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_extension_function_address_params_t
{
    ze_driver_handle_t* phDriver;
    const char** ppFuncName;
    void*** ppfunc;
} ze_driver_get_extension_function_address_params_t;

typedef void (__zecall *ze_pfnDriverGetExtensionFunctionAddressCb_t)(
    ze_driver_get_extension_function_address_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_alloc_shared_mem_params_t
{
    ze_driver_handle_t* phDriver;
    ze_device_handle_t* phDevice;
    ze_device_mem_alloc_flag_t* pdevice_flags;
    uint32_t* pordinal;
    ze_host_mem_alloc_flag_t* phost_flags;
    size_t* psize;
    size_t* palignment;
    void*** ppptr;
} ze_driver_alloc_shared_mem_params_t;

typedef void (__zecall *ze_pfnDriverAllocSharedMemCb_t)(
    ze_driver_alloc_shared_mem_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_alloc_device_mem_params_t
{
    ze_driver_handle_t* phDriver;
    ze_device_handle_t* phDevice;
    ze_device_mem_alloc_flag_t* pflags;
    uint32_t* pordinal;
    size_t* psize;
    size_t* palignment;
    void*** ppptr;
} ze_driver_alloc_device_mem_params_t;

typedef void (__zecall *ze_pfnDriverAllocDeviceMemCb_t)(
    ze_driver_alloc_device_mem_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_alloc_host_mem_params_t
{
    ze_driver_handle_t* phDriver;
    ze_host_mem_alloc_flag_t* pflags;
    size_t* psize;
    size_t* palignment;
    void*** ppptr;
} ze_driver_alloc_host_mem_params_t;

typedef void (__zecall *ze_pfnDriverAllocHostMemCb_t)(
    ze_driver_alloc_host_mem_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_free_mem_params_t
{
    ze_driver_handle_t* phDriver;
    void** pptr;
} ze_driver_free_mem_params_t;

typedef void (__zecall *ze_pfnDriverFreeMemCb_t)(
    ze_driver_free_mem_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_mem_alloc_properties_params_t
{
    ze_driver_handle_t* phDriver;
    const void** pptr;
    ze_memory_allocation_properties_t** ppMemAllocProperties;
    ze_device_handle_t** pphDevice;
} ze_driver_get_mem_alloc_properties_params_t;

typedef void (__zecall *ze_pfnDriverGetMemAllocPropertiesCb_t)(
    ze_driver_get_mem_alloc_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_mem_address_range_params_t
{
    ze_driver_handle_t* phDriver;
    const void** pptr;
    void*** ppBase;
    size_t** ppSize;
} ze_driver_get_mem_address_range_params_t;

typedef void (__zecall *ze_pfnDriverGetMemAddressRangeCb_t)(
    ze_driver_get_mem_address_range_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_get_mem_ipc_handle_params_t
{
    ze_driver_handle_t* phDriver;
    const void** pptr;
    ze_ipc_mem_handle_t** ppIpcHandle;
} ze_driver_get_mem_ipc_handle_params_t;

typedef void (__zecall *ze_pfnDriverGetMemIpcHandleCb_t)(
    ze_driver_get_mem_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_open_mem_ipc_handle_params_t
{
    ze_driver_handle_t* phDriver;
    ze_device_handle_t* phDevice;
    ze_ipc_mem_handle_t* phandle;
    ze_ipc_memory_flag_t* pflags;
    void*** ppptr;
} ze_driver_open_mem_ipc_handle_params_t;

typedef void (__zecall *ze_pfnDriverOpenMemIpcHandleCb_t)(
    ze_driver_open_mem_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_close_mem_ipc_handle_params_t
{
    ze_driver_handle_t* phDriver;
    const void** pptr;
} ze_driver_close_mem_ipc_handle_params_t;

typedef void (__zecall *ze_pfnDriverCloseMemIpcHandleCb_t)(
    ze_driver_close_mem_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_driver_callbacks_t
{
    ze_pfnDriverGetCb_t                                             pfnGetCb;
    ze_pfnDriverGetDriverVersionCb_t                                pfnGetDriverVersionCb;
    ze_pfnDriverGetApiVersionCb_t                                   pfnGetApiVersionCb;
    ze_pfnDriverGetIPCPropertiesCb_t                                pfnGetIPCPropertiesCb;
    ze_pfnDriverGetExtensionFunctionAddressCb_t                     pfnGetExtensionFunctionAddressCb;
    ze_pfnDriverAllocSharedMemCb_t                                  pfnAllocSharedMemCb;
    ze_pfnDriverAllocDeviceMemCb_t                                  pfnAllocDeviceMemCb;
    ze_pfnDriverAllocHostMemCb_t                                    pfnAllocHostMemCb;
    ze_pfnDriverFreeMemCb_t                                         pfnFreeMemCb;
    ze_pfnDriverGetMemAllocPropertiesCb_t                           pfnGetMemAllocPropertiesCb;
    ze_pfnDriverGetMemAddressRangeCb_t                              pfnGetMemAddressRangeCb;
    ze_pfnDriverGetMemIpcHandleCb_t                                 pfnGetMemIpcHandleCb;
    ze_pfnDriverOpenMemIpcHandleCb_t                                pfnOpenMemIpcHandleCb;
    ze_pfnDriverCloseMemIpcHandleCb_t                               pfnCloseMemIpcHandleCb;
} ze_driver_callbacks_t;

typedef struct _ze_command_queue_create_params_t
{
    ze_device_handle_t* phDevice;
    const ze_command_queue_desc_t** pdesc;
    ze_command_queue_handle_t** pphCommandQueue;
} ze_command_queue_create_params_t;

typedef void (__zecall *ze_pfnCommandQueueCreateCb_t)(
    ze_command_queue_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_queue_destroy_params_t
{
    ze_command_queue_handle_t* phCommandQueue;
} ze_command_queue_destroy_params_t;

typedef void (__zecall *ze_pfnCommandQueueDestroyCb_t)(
    ze_command_queue_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_queue_execute_command_lists_params_t
{
    ze_command_queue_handle_t* phCommandQueue;
    uint32_t* pnumCommandLists;
    ze_command_list_handle_t** pphCommandLists;
    ze_fence_handle_t* phFence;
} ze_command_queue_execute_command_lists_params_t;

typedef void (__zecall *ze_pfnCommandQueueExecuteCommandListsCb_t)(
    ze_command_queue_execute_command_lists_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_queue_synchronize_params_t
{
    ze_command_queue_handle_t* phCommandQueue;
    uint32_t* ptimeout;
} ze_command_queue_synchronize_params_t;

typedef void (__zecall *ze_pfnCommandQueueSynchronizeCb_t)(
    ze_command_queue_synchronize_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_queue_callbacks_t
{
    ze_pfnCommandQueueCreateCb_t                                    pfnCreateCb;
    ze_pfnCommandQueueDestroyCb_t                                   pfnDestroyCb;
    ze_pfnCommandQueueExecuteCommandListsCb_t                       pfnExecuteCommandListsCb;
    ze_pfnCommandQueueSynchronizeCb_t                               pfnSynchronizeCb;
} ze_command_queue_callbacks_t;

typedef struct _ze_command_list_create_params_t
{
    ze_device_handle_t* phDevice;
    const ze_command_list_desc_t** pdesc;
    ze_command_list_handle_t** pphCommandList;
} ze_command_list_create_params_t;

typedef void (__zecall *ze_pfnCommandListCreateCb_t)(
    ze_command_list_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_create_immediate_params_t
{
    ze_device_handle_t* phDevice;
    const ze_command_queue_desc_t** paltdesc;
    ze_command_list_handle_t** pphCommandList;
} ze_command_list_create_immediate_params_t;

typedef void (__zecall *ze_pfnCommandListCreateImmediateCb_t)(
    ze_command_list_create_immediate_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_destroy_params_t
{
    ze_command_list_handle_t* phCommandList;
} ze_command_list_destroy_params_t;

typedef void (__zecall *ze_pfnCommandListDestroyCb_t)(
    ze_command_list_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_close_params_t
{
    ze_command_list_handle_t* phCommandList;
} ze_command_list_close_params_t;

typedef void (__zecall *ze_pfnCommandListCloseCb_t)(
    ze_command_list_close_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_reset_params_t
{
    ze_command_list_handle_t* phCommandList;
} ze_command_list_reset_params_t;

typedef void (__zecall *ze_pfnCommandListResetCb_t)(
    ze_command_list_reset_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_barrier_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_barrier_params_t;

typedef void (__zecall *ze_pfnCommandListAppendBarrierCb_t)(
    ze_command_list_append_barrier_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_memory_ranges_barrier_params_t
{
    ze_command_list_handle_t* phCommandList;
    uint32_t* pnumRanges;
    const size_t** ppRangeSizes;
    const void*** ppRanges;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_memory_ranges_barrier_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemoryRangesBarrierCb_t)(
    ze_command_list_append_memory_ranges_barrier_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_memory_copy_params_t
{
    ze_command_list_handle_t* phCommandList;
    void** pdstptr;
    const void** psrcptr;
    size_t* psize;
    ze_event_handle_t* phEvent;
} ze_command_list_append_memory_copy_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemoryCopyCb_t)(
    ze_command_list_append_memory_copy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_memory_set_params_t
{
    ze_command_list_handle_t* phCommandList;
    void** pptr;
    int* pvalue;
    size_t* psize;
    ze_event_handle_t* phEvent;
} ze_command_list_append_memory_set_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemorySetCb_t)(
    ze_command_list_append_memory_set_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_memory_copy_region_params_t
{
    ze_command_list_handle_t* phCommandList;
    void** pdstptr;
    const ze_copy_region_t** pdstRegion;
    uint32_t* pdstPitch;
    const void** psrcptr;
    const ze_copy_region_t** psrcRegion;
    uint32_t* psrcPitch;
    ze_event_handle_t* phEvent;
} ze_command_list_append_memory_copy_region_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemoryCopyRegionCb_t)(
    ze_command_list_append_memory_copy_region_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_image_copy_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_image_handle_t* phDstImage;
    ze_image_handle_t* phSrcImage;
    ze_event_handle_t* phEvent;
} ze_command_list_append_image_copy_params_t;

typedef void (__zecall *ze_pfnCommandListAppendImageCopyCb_t)(
    ze_command_list_append_image_copy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_image_copy_region_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_image_handle_t* phDstImage;
    ze_image_handle_t* phSrcImage;
    const ze_image_region_t** ppDstRegion;
    const ze_image_region_t** ppSrcRegion;
    ze_event_handle_t* phEvent;
} ze_command_list_append_image_copy_region_params_t;

typedef void (__zecall *ze_pfnCommandListAppendImageCopyRegionCb_t)(
    ze_command_list_append_image_copy_region_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_image_copy_to_memory_params_t
{
    ze_command_list_handle_t* phCommandList;
    void** pdstptr;
    ze_image_handle_t* phSrcImage;
    const ze_image_region_t** ppSrcRegion;
    ze_event_handle_t* phEvent;
} ze_command_list_append_image_copy_to_memory_params_t;

typedef void (__zecall *ze_pfnCommandListAppendImageCopyToMemoryCb_t)(
    ze_command_list_append_image_copy_to_memory_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_image_copy_from_memory_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_image_handle_t* phDstImage;
    const void** psrcptr;
    const ze_image_region_t** ppDstRegion;
    ze_event_handle_t* phEvent;
} ze_command_list_append_image_copy_from_memory_params_t;

typedef void (__zecall *ze_pfnCommandListAppendImageCopyFromMemoryCb_t)(
    ze_command_list_append_image_copy_from_memory_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_memory_prefetch_params_t
{
    ze_command_list_handle_t* phCommandList;
    const void** pptr;
    size_t* psize;
} ze_command_list_append_memory_prefetch_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemoryPrefetchCb_t)(
    ze_command_list_append_memory_prefetch_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_mem_advise_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_device_handle_t* phDevice;
    const void** pptr;
    size_t* psize;
    ze_memory_advice_t* padvice;
} ze_command_list_append_mem_advise_params_t;

typedef void (__zecall *ze_pfnCommandListAppendMemAdviseCb_t)(
    ze_command_list_append_mem_advise_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_signal_event_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_event_handle_t* phEvent;
} ze_command_list_append_signal_event_params_t;

typedef void (__zecall *ze_pfnCommandListAppendSignalEventCb_t)(
    ze_command_list_append_signal_event_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_wait_on_events_params_t
{
    ze_command_list_handle_t* phCommandList;
    uint32_t* pnumEvents;
    ze_event_handle_t** pphEvents;
} ze_command_list_append_wait_on_events_params_t;

typedef void (__zecall *ze_pfnCommandListAppendWaitOnEventsCb_t)(
    ze_command_list_append_wait_on_events_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_event_reset_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_event_handle_t* phEvent;
} ze_command_list_append_event_reset_params_t;

typedef void (__zecall *ze_pfnCommandListAppendEventResetCb_t)(
    ze_command_list_append_event_reset_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_launch_kernel_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_kernel_handle_t* phKernel;
    const ze_thread_group_dimensions_t** ppLaunchFuncArgs;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_launch_kernel_params_t;

typedef void (__zecall *ze_pfnCommandListAppendLaunchKernelCb_t)(
    ze_command_list_append_launch_kernel_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_launch_cooperative_kernel_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_kernel_handle_t* phKernel;
    const ze_thread_group_dimensions_t** ppLaunchFuncArgs;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_launch_cooperative_kernel_params_t;

typedef void (__zecall *ze_pfnCommandListAppendLaunchCooperativeKernelCb_t)(
    ze_command_list_append_launch_cooperative_kernel_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_launch_kernel_indirect_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_kernel_handle_t* phKernel;
    const ze_thread_group_dimensions_t** ppLaunchArgumentsBuffer;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_launch_kernel_indirect_params_t;

typedef void (__zecall *ze_pfnCommandListAppendLaunchKernelIndirectCb_t)(
    ze_command_list_append_launch_kernel_indirect_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_launch_multiple_kernels_indirect_params_t
{
    ze_command_list_handle_t* phCommandList;
    uint32_t* pnumKernels;
    ze_kernel_handle_t** pphKernels;
    const uint32_t** ppCountBuffer;
    const ze_thread_group_dimensions_t** ppLaunchArgumentsBuffer;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_launch_multiple_kernels_indirect_params_t;

typedef void (__zecall *ze_pfnCommandListAppendLaunchMultipleKernelsIndirectCb_t)(
    ze_command_list_append_launch_multiple_kernels_indirect_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_append_launch_host_function_params_t
{
    ze_command_list_handle_t* phCommandList;
    ze_host_pfn_t* ppfnHostFunc;
    void** ppUserData;
    ze_event_handle_t* phSignalEvent;
    uint32_t* pnumWaitEvents;
    ze_event_handle_t** pphWaitEvents;
} ze_command_list_append_launch_host_function_params_t;

typedef void (__zecall *ze_pfnCommandListAppendLaunchHostFunctionCb_t)(
    ze_command_list_append_launch_host_function_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_command_list_callbacks_t
{
    ze_pfnCommandListCreateCb_t                                     pfnCreateCb;
    ze_pfnCommandListCreateImmediateCb_t                            pfnCreateImmediateCb;
    ze_pfnCommandListDestroyCb_t                                    pfnDestroyCb;
    ze_pfnCommandListCloseCb_t                                      pfnCloseCb;
    ze_pfnCommandListResetCb_t                                      pfnResetCb;
    ze_pfnCommandListAppendBarrierCb_t                              pfnAppendBarrierCb;
    ze_pfnCommandListAppendMemoryRangesBarrierCb_t                  pfnAppendMemoryRangesBarrierCb;
    ze_pfnCommandListAppendMemoryCopyCb_t                           pfnAppendMemoryCopyCb;
    ze_pfnCommandListAppendMemorySetCb_t                            pfnAppendMemorySetCb;
    ze_pfnCommandListAppendMemoryCopyRegionCb_t                     pfnAppendMemoryCopyRegionCb;
    ze_pfnCommandListAppendImageCopyCb_t                            pfnAppendImageCopyCb;
    ze_pfnCommandListAppendImageCopyRegionCb_t                      pfnAppendImageCopyRegionCb;
    ze_pfnCommandListAppendImageCopyToMemoryCb_t                    pfnAppendImageCopyToMemoryCb;
    ze_pfnCommandListAppendImageCopyFromMemoryCb_t                  pfnAppendImageCopyFromMemoryCb;
    ze_pfnCommandListAppendMemoryPrefetchCb_t                       pfnAppendMemoryPrefetchCb;
    ze_pfnCommandListAppendMemAdviseCb_t                            pfnAppendMemAdviseCb;
    ze_pfnCommandListAppendSignalEventCb_t                          pfnAppendSignalEventCb;
    ze_pfnCommandListAppendWaitOnEventsCb_t                         pfnAppendWaitOnEventsCb;
    ze_pfnCommandListAppendEventResetCb_t                           pfnAppendEventResetCb;
    ze_pfnCommandListAppendLaunchKernelCb_t                         pfnAppendLaunchKernelCb;
    ze_pfnCommandListAppendLaunchCooperativeKernelCb_t              pfnAppendLaunchCooperativeKernelCb;
    ze_pfnCommandListAppendLaunchKernelIndirectCb_t                 pfnAppendLaunchKernelIndirectCb;
    ze_pfnCommandListAppendLaunchMultipleKernelsIndirectCb_t        pfnAppendLaunchMultipleKernelsIndirectCb;
    ze_pfnCommandListAppendLaunchHostFunctionCb_t                   pfnAppendLaunchHostFunctionCb;
} ze_command_list_callbacks_t;

typedef struct _ze_fence_create_params_t
{
    ze_command_queue_handle_t* phCommandQueue;
    const ze_fence_desc_t** pdesc;
    ze_fence_handle_t** pphFence;
} ze_fence_create_params_t;

typedef void (__zecall *ze_pfnFenceCreateCb_t)(
    ze_fence_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_fence_destroy_params_t
{
    ze_fence_handle_t* phFence;
} ze_fence_destroy_params_t;

typedef void (__zecall *ze_pfnFenceDestroyCb_t)(
    ze_fence_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_fence_host_synchronize_params_t
{
    ze_fence_handle_t* phFence;
    uint32_t* ptimeout;
} ze_fence_host_synchronize_params_t;

typedef void (__zecall *ze_pfnFenceHostSynchronizeCb_t)(
    ze_fence_host_synchronize_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_fence_query_status_params_t
{
    ze_fence_handle_t* phFence;
} ze_fence_query_status_params_t;

typedef void (__zecall *ze_pfnFenceQueryStatusCb_t)(
    ze_fence_query_status_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_fence_reset_params_t
{
    ze_fence_handle_t* phFence;
} ze_fence_reset_params_t;

typedef void (__zecall *ze_pfnFenceResetCb_t)(
    ze_fence_reset_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_fence_callbacks_t
{
    ze_pfnFenceCreateCb_t                                           pfnCreateCb;
    ze_pfnFenceDestroyCb_t                                          pfnDestroyCb;
    ze_pfnFenceHostSynchronizeCb_t                                  pfnHostSynchronizeCb;
    ze_pfnFenceQueryStatusCb_t                                      pfnQueryStatusCb;
    ze_pfnFenceResetCb_t                                            pfnResetCb;
} ze_fence_callbacks_t;

typedef struct _ze_event_pool_create_params_t
{
    ze_driver_handle_t* phDriver;
    const ze_event_pool_desc_t** pdesc;
    uint32_t* pnumDevices;
    ze_device_handle_t** pphDevices;
    ze_event_pool_handle_t** pphEventPool;
} ze_event_pool_create_params_t;

typedef void (__zecall *ze_pfnEventPoolCreateCb_t)(
    ze_event_pool_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_pool_destroy_params_t
{
    ze_event_pool_handle_t* phEventPool;
} ze_event_pool_destroy_params_t;

typedef void (__zecall *ze_pfnEventPoolDestroyCb_t)(
    ze_event_pool_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_pool_get_ipc_handle_params_t
{
    ze_event_pool_handle_t* phEventPool;
    ze_ipc_event_pool_handle_t** pphIpc;
} ze_event_pool_get_ipc_handle_params_t;

typedef void (__zecall *ze_pfnEventPoolGetIpcHandleCb_t)(
    ze_event_pool_get_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_pool_open_ipc_handle_params_t
{
    ze_driver_handle_t* phDriver;
    ze_ipc_event_pool_handle_t* phIpc;
    ze_event_pool_handle_t** pphEventPool;
} ze_event_pool_open_ipc_handle_params_t;

typedef void (__zecall *ze_pfnEventPoolOpenIpcHandleCb_t)(
    ze_event_pool_open_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_pool_close_ipc_handle_params_t
{
    ze_event_pool_handle_t* phEventPool;
} ze_event_pool_close_ipc_handle_params_t;

typedef void (__zecall *ze_pfnEventPoolCloseIpcHandleCb_t)(
    ze_event_pool_close_ipc_handle_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_pool_callbacks_t
{
    ze_pfnEventPoolCreateCb_t                                       pfnCreateCb;
    ze_pfnEventPoolDestroyCb_t                                      pfnDestroyCb;
    ze_pfnEventPoolGetIpcHandleCb_t                                 pfnGetIpcHandleCb;
    ze_pfnEventPoolOpenIpcHandleCb_t                                pfnOpenIpcHandleCb;
    ze_pfnEventPoolCloseIpcHandleCb_t                               pfnCloseIpcHandleCb;
} ze_event_pool_callbacks_t;

typedef struct _ze_event_create_params_t
{
    ze_event_pool_handle_t* phEventPool;
    const ze_event_desc_t** pdesc;
    ze_event_handle_t** pphEvent;
} ze_event_create_params_t;

typedef void (__zecall *ze_pfnEventCreateCb_t)(
    ze_event_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_destroy_params_t
{
    ze_event_handle_t* phEvent;
} ze_event_destroy_params_t;

typedef void (__zecall *ze_pfnEventDestroyCb_t)(
    ze_event_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_host_signal_params_t
{
    ze_event_handle_t* phEvent;
} ze_event_host_signal_params_t;

typedef void (__zecall *ze_pfnEventHostSignalCb_t)(
    ze_event_host_signal_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_host_synchronize_params_t
{
    ze_event_handle_t* phEvent;
    uint32_t* ptimeout;
} ze_event_host_synchronize_params_t;

typedef void (__zecall *ze_pfnEventHostSynchronizeCb_t)(
    ze_event_host_synchronize_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_query_status_params_t
{
    ze_event_handle_t* phEvent;
} ze_event_query_status_params_t;

typedef void (__zecall *ze_pfnEventQueryStatusCb_t)(
    ze_event_query_status_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_reset_params_t
{
    ze_event_handle_t* phEvent;
} ze_event_reset_params_t;

typedef void (__zecall *ze_pfnEventResetCb_t)(
    ze_event_reset_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_event_callbacks_t
{
    ze_pfnEventCreateCb_t                                           pfnCreateCb;
    ze_pfnEventDestroyCb_t                                          pfnDestroyCb;
    ze_pfnEventHostSignalCb_t                                       pfnHostSignalCb;
    ze_pfnEventHostSynchronizeCb_t                                  pfnHostSynchronizeCb;
    ze_pfnEventQueryStatusCb_t                                      pfnQueryStatusCb;
    ze_pfnEventResetCb_t                                            pfnResetCb;
} ze_event_callbacks_t;

typedef struct _ze_image_get_properties_params_t
{
    ze_device_handle_t* phDevice;
    const ze_image_desc_t** pdesc;
    ze_image_properties_t** ppImageProperties;
} ze_image_get_properties_params_t;

typedef void (__zecall *ze_pfnImageGetPropertiesCb_t)(
    ze_image_get_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_image_create_params_t
{
    ze_device_handle_t* phDevice;
    const ze_image_desc_t** pdesc;
    ze_image_handle_t** pphImage;
} ze_image_create_params_t;

typedef void (__zecall *ze_pfnImageCreateCb_t)(
    ze_image_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_image_destroy_params_t
{
    ze_image_handle_t* phImage;
} ze_image_destroy_params_t;

typedef void (__zecall *ze_pfnImageDestroyCb_t)(
    ze_image_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_image_callbacks_t
{
    ze_pfnImageGetPropertiesCb_t                                    pfnGetPropertiesCb;
    ze_pfnImageCreateCb_t                                           pfnCreateCb;
    ze_pfnImageDestroyCb_t                                          pfnDestroyCb;
} ze_image_callbacks_t;

typedef struct _ze_module_create_params_t
{
    ze_device_handle_t* phDevice;
    const ze_module_desc_t** pdesc;
    ze_module_handle_t** pphModule;
    ze_module_build_log_handle_t** pphBuildLog;
} ze_module_create_params_t;

typedef void (__zecall *ze_pfnModuleCreateCb_t)(
    ze_module_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_destroy_params_t
{
    ze_module_handle_t* phModule;
} ze_module_destroy_params_t;

typedef void (__zecall *ze_pfnModuleDestroyCb_t)(
    ze_module_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_get_native_binary_params_t
{
    ze_module_handle_t* phModule;
    size_t** ppSize;
    uint8_t** ppModuleNativeBinary;
} ze_module_get_native_binary_params_t;

typedef void (__zecall *ze_pfnModuleGetNativeBinaryCb_t)(
    ze_module_get_native_binary_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_get_global_pointer_params_t
{
    ze_module_handle_t* phModule;
    const char** ppGlobalName;
    void*** ppptr;
} ze_module_get_global_pointer_params_t;

typedef void (__zecall *ze_pfnModuleGetGlobalPointerCb_t)(
    ze_module_get_global_pointer_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_get_function_pointer_params_t
{
    ze_module_handle_t* phModule;
    const char** ppFunctionName;
    void*** ppfnFunction;
} ze_module_get_function_pointer_params_t;

typedef void (__zecall *ze_pfnModuleGetFunctionPointerCb_t)(
    ze_module_get_function_pointer_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_callbacks_t
{
    ze_pfnModuleCreateCb_t                                          pfnCreateCb;
    ze_pfnModuleDestroyCb_t                                         pfnDestroyCb;
    ze_pfnModuleGetNativeBinaryCb_t                                 pfnGetNativeBinaryCb;
    ze_pfnModuleGetGlobalPointerCb_t                                pfnGetGlobalPointerCb;
    ze_pfnModuleGetFunctionPointerCb_t                              pfnGetFunctionPointerCb;
} ze_module_callbacks_t;

typedef struct _ze_module_build_log_destroy_params_t
{
    ze_module_build_log_handle_t* phModuleBuildLog;
} ze_module_build_log_destroy_params_t;

typedef void (__zecall *ze_pfnModuleBuildLogDestroyCb_t)(
    ze_module_build_log_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_build_log_get_string_params_t
{
    ze_module_build_log_handle_t* phModuleBuildLog;
    size_t** ppSize;
    char** ppBuildLog;
} ze_module_build_log_get_string_params_t;

typedef void (__zecall *ze_pfnModuleBuildLogGetStringCb_t)(
    ze_module_build_log_get_string_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_module_build_log_callbacks_t
{
    ze_pfnModuleBuildLogDestroyCb_t                                 pfnDestroyCb;
    ze_pfnModuleBuildLogGetStringCb_t                               pfnGetStringCb;
} ze_module_build_log_callbacks_t;

typedef struct _ze_kernel_create_params_t
{
    ze_module_handle_t* phModule;
    const ze_kernel_desc_t** pdesc;
    ze_kernel_handle_t** pphKernel;
} ze_kernel_create_params_t;

typedef void (__zecall *ze_pfnKernelCreateCb_t)(
    ze_kernel_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_destroy_params_t
{
    ze_kernel_handle_t* phKernel;
} ze_kernel_destroy_params_t;

typedef void (__zecall *ze_pfnKernelDestroyCb_t)(
    ze_kernel_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_set_intermediate_cache_config_params_t
{
    ze_kernel_handle_t* phKernel;
    ze_cache_config_t* pCacheConfig;
} ze_kernel_set_intermediate_cache_config_params_t;

typedef void (__zecall *ze_pfnKernelSetIntermediateCacheConfigCb_t)(
    ze_kernel_set_intermediate_cache_config_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_set_group_size_params_t
{
    ze_kernel_handle_t* phKernel;
    uint32_t* pgroupSizeX;
    uint32_t* pgroupSizeY;
    uint32_t* pgroupSizeZ;
} ze_kernel_set_group_size_params_t;

typedef void (__zecall *ze_pfnKernelSetGroupSizeCb_t)(
    ze_kernel_set_group_size_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_suggest_group_size_params_t
{
    ze_kernel_handle_t* phKernel;
    uint32_t* pglobalSizeX;
    uint32_t* pglobalSizeY;
    uint32_t* pglobalSizeZ;
    uint32_t** pgroupSizeX;
    uint32_t** pgroupSizeY;
    uint32_t** pgroupSizeZ;
} ze_kernel_suggest_group_size_params_t;

typedef void (__zecall *ze_pfnKernelSuggestGroupSizeCb_t)(
    ze_kernel_suggest_group_size_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_suggest_max_cooperative_group_count_params_t
{
    ze_kernel_handle_t* phKernel;
    uint32_t** ptotalGroupCount;
} ze_kernel_suggest_max_cooperative_group_count_params_t;

typedef void (__zecall *ze_pfnKernelSuggestMaxCooperativeGroupCountCb_t)(
    ze_kernel_suggest_max_cooperative_group_count_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_set_argument_value_params_t
{
    ze_kernel_handle_t* phKernel;
    uint32_t* pargIndex;
    size_t* pargSize;
    const void** ppArgValue;
} ze_kernel_set_argument_value_params_t;

typedef void (__zecall *ze_pfnKernelSetArgumentValueCb_t)(
    ze_kernel_set_argument_value_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_set_attribute_params_t
{
    ze_kernel_handle_t* phKernel;
    ze_kernel_set_attribute_t* pattr;
    uint32_t* pvalue;
} ze_kernel_set_attribute_params_t;

typedef void (__zecall *ze_pfnKernelSetAttributeCb_t)(
    ze_kernel_set_attribute_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_get_properties_params_t
{
    ze_kernel_handle_t* phKernel;
    ze_kernel_properties_t** ppKernelProperties;
} ze_kernel_get_properties_params_t;

typedef void (__zecall *ze_pfnKernelGetPropertiesCb_t)(
    ze_kernel_get_properties_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_kernel_callbacks_t
{
    ze_pfnKernelCreateCb_t                                          pfnCreateCb;
    ze_pfnKernelDestroyCb_t                                         pfnDestroyCb;
    ze_pfnKernelSetIntermediateCacheConfigCb_t                      pfnSetIntermediateCacheConfigCb;
    ze_pfnKernelSetGroupSizeCb_t                                    pfnSetGroupSizeCb;
    ze_pfnKernelSuggestGroupSizeCb_t                                pfnSuggestGroupSizeCb;
    ze_pfnKernelSuggestMaxCooperativeGroupCountCb_t                 pfnSuggestMaxCooperativeGroupCountCb;
    ze_pfnKernelSetArgumentValueCb_t                                pfnSetArgumentValueCb;
    ze_pfnKernelSetAttributeCb_t                                    pfnSetAttributeCb;
    ze_pfnKernelGetPropertiesCb_t                                   pfnGetPropertiesCb;
} ze_kernel_callbacks_t;

typedef struct _ze_sampler_create_params_t
{
    ze_device_handle_t* phDevice;
    const ze_sampler_desc_t** pdesc;
    ze_sampler_handle_t** pphSampler;
} ze_sampler_create_params_t;

typedef void (__zecall *ze_pfnSamplerCreateCb_t)(
    ze_sampler_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_sampler_destroy_params_t
{
    ze_sampler_handle_t* phSampler;
} ze_sampler_destroy_params_t;

typedef void (__zecall *ze_pfnSamplerDestroyCb_t)(
    ze_sampler_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _ze_sampler_callbacks_t
{
    ze_pfnSamplerCreateCb_t                                         pfnCreateCb;
    ze_pfnSamplerDestroyCb_t                                        pfnDestroyCb;
} ze_sampler_callbacks_t;

typedef struct _ze_callbacks_t
{
    ze_global_callbacks_t               Global;
    ze_device_callbacks_t               Device;
    ze_driver_callbacks_t               Driver;
    ze_command_queue_callbacks_t        CommandQueue;
    ze_command_list_callbacks_t         CommandList;
    ze_fence_callbacks_t                Fence;
    ze_event_pool_callbacks_t           EventPool;
    ze_event_callbacks_t                Event;
    ze_image_callbacks_t                Image;
    ze_module_callbacks_t               Module;
    ze_module_build_log_callbacks_t     ModuleBuildLog;
    ze_kernel_callbacks_t               Kernel;
    ze_sampler_callbacks_t              Sampler;
} ze_callbacks_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_CALLBACKS_H
