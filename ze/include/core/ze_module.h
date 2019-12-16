/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_module.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Module
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/module.yml
 * @endcond
 *
 */
#ifndef _ZE_MODULE_H
#define _ZE_MODULE_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_module_desc_version_t
{
    ZE_MODULE_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_module_desc_version_t;

typedef enum _ze_module_format_t
{
    ZE_MODULE_FORMAT_IL_SPIRV = 0,
    ZE_MODULE_FORMAT_NATIVE,

} ze_module_format_t;

typedef struct _ze_module_constants_t
{
    uint32_t numConstants;
    const uint32_t* pConstantIds;
    const uint64_t* pConstantValues;

} ze_module_constants_t;

typedef struct _ze_module_desc_t
{
    ze_module_desc_version_t version;
    ze_module_format_t format;
    size_t inputSize;
    const uint8_t* pInputModule;
    const char* pBuildFlags;
    const ze_module_constants_t* pConstants;

} ze_module_desc_t;

ze_result_t __zecall
zeModuleCreate(
    ze_device_handle_t hDevice,
    const ze_module_desc_t* desc,
    ze_module_handle_t* phModule,
    ze_module_build_log_handle_t* phBuildLog
    );

ze_result_t __zecall
zeModuleDestroy(
    ze_module_handle_t hModule
    );

ze_result_t __zecall
zeModuleBuildLogDestroy(
    ze_module_build_log_handle_t hModuleBuildLog
    );

ze_result_t __zecall
zeModuleBuildLogGetString(
    ze_module_build_log_handle_t hModuleBuildLog,
    size_t* pSize,
    char* pBuildLog
    );

ze_result_t __zecall
zeModuleGetNativeBinary(
    ze_module_handle_t hModule,
    size_t* pSize,
    uint8_t* pModuleNativeBinary
    );

ze_result_t __zecall
zeModuleGetGlobalPointer(
    ze_module_handle_t hModule,
    const char* pGlobalName,
    void** pptr
    );

typedef enum _ze_kernel_desc_version_t
{
    ZE_KERNEL_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_kernel_desc_version_t;

typedef enum _ze_kernel_flag_t
{
    ZE_KERNEL_FLAG_NONE = 0,
    ZE_KERNEL_FLAG_FORCE_RESIDENCY,

} ze_kernel_flag_t;

typedef struct _ze_kernel_desc_t
{
    ze_kernel_desc_version_t version;
    ze_kernel_flag_t flags;
    const char* pKernelName;

} ze_kernel_desc_t;

ze_result_t __zecall
zeKernelCreate(
    ze_module_handle_t hModule,
    const ze_kernel_desc_t* desc,
    ze_kernel_handle_t* phKernel
    );

ze_result_t __zecall
zeKernelDestroy(
    ze_kernel_handle_t hKernel
    );

ze_result_t __zecall
zeModuleGetFunctionPointer(
    ze_module_handle_t hModule,
    const char* pFunctionName,
    void** pfnFunction
    );

ze_result_t __zecall
zeKernelSetGroupSize(
    ze_kernel_handle_t hKernel,
    uint32_t groupSizeX,
    uint32_t groupSizeY,
    uint32_t groupSizeZ
    );

ze_result_t __zecall
zeKernelSuggestGroupSize(
    ze_kernel_handle_t hKernel,
    uint32_t globalSizeX,
    uint32_t globalSizeY,
    uint32_t globalSizeZ,
    uint32_t* groupSizeX,
    uint32_t* groupSizeY,
    uint32_t* groupSizeZ
    );

ze_result_t __zecall
zeKernelSuggestMaxCooperativeGroupCount(
    ze_kernel_handle_t hKernel,
    uint32_t* totalGroupCount
    );

ze_result_t __zecall
zeKernelSetArgumentValue(
    ze_kernel_handle_t hKernel,
    uint32_t argIndex,
    size_t argSize,
    const void* pArgValue
    );

typedef enum _ze_kernel_set_attribute_t
{
    ZE_KERNEL_SET_ATTR_INDIRECT_HOST_ACCESS = 0,
    ZE_KERNEL_SET_ATTR_INDIRECT_DEVICE_ACCESS,
    ZE_KERNEL_SET_ATTR_INDIRECT_SHARED_ACCESS,

} ze_kernel_set_attribute_t;

ze_result_t __zecall
zeKernelSetAttribute(
    ze_kernel_handle_t hKernel,
    ze_kernel_set_attribute_t attr,
    uint32_t value
    );

ze_result_t __zecall
zeKernelSetIntermediateCacheConfig(
    ze_kernel_handle_t hKernel,
    ze_cache_config_t CacheConfig
    );

typedef struct _ze_thread_group_dimensions_t
{
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

} ze_thread_group_dimensions_t;

typedef enum _ze_kernel_properties_version_t
{
    ZE_KERNEL_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_kernel_properties_version_t;

#ifndef ZE_MAX_KERNEL_NAME
#define ZE_MAX_KERNEL_NAME  256
#endif // ZE_MAX_KERNEL_NAME

typedef struct _ze_kernel_properties_t
{
    ze_kernel_properties_version_t version;
    char name[ZE_MAX_KERNEL_NAME];
    uint32_t numKernelArgs;
    ze_thread_group_dimensions_t compileGroupSize;

} ze_kernel_properties_t;

ze_result_t __zecall
zeKernelGetProperties(
    ze_kernel_handle_t hKernel,
    ze_kernel_properties_t* pKernelProperties
    );

ze_result_t __zecall
zeCommandListAppendLaunchKernel(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_thread_group_dimensions_t* pLaunchFuncArgs,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

ze_result_t __zecall
zeCommandListAppendLaunchCooperativeKernel(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_thread_group_dimensions_t* pLaunchFuncArgs,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

ze_result_t __zecall
zeCommandListAppendLaunchKernelIndirect(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_thread_group_dimensions_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

ze_result_t __zecall
zeCommandListAppendLaunchMultipleKernelsIndirect(
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    const uint32_t* pCountBuffer,
    const ze_thread_group_dimensions_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

typedef void(__zecall *ze_host_pfn_t)(
    void* pUserData
    );

ze_result_t __zecall
zeCommandListAppendLaunchHostFunction(
    ze_command_list_handle_t hCommandList,
    ze_host_pfn_t pfnHostFunc,
    void* pUserData,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_MODULE_H
