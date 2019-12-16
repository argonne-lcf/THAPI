/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_memory.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Memory
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/memory.yml
 * @endcond
 *
 */
#ifndef _ZE_MEMORY_H
#define _ZE_MEMORY_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_device_mem_alloc_flag_t
{
    ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT = 0,
    ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED = ZE_BIT( 0 ),
    ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED = ZE_BIT( 1 ),

} ze_device_mem_alloc_flag_t;

typedef enum _ze_host_mem_alloc_flag_t
{
    ZE_HOST_MEM_ALLOC_FLAG_DEFAULT = 0,
    ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED = ZE_BIT( 0 ),
    ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED = ZE_BIT( 1 ),
    ZE_HOST_MEM_ALLOC_FLAG_BIAS_WRITE_COMBINED = ZE_BIT( 2 ),

} ze_host_mem_alloc_flag_t;

ze_result_t __zecall
zeDriverAllocSharedMem(
    ze_driver_handle_t hDriver,
    ze_device_handle_t hDevice,
    ze_device_mem_alloc_flag_t device_flags,
    uint32_t ordinal,
    ze_host_mem_alloc_flag_t host_flags,
    size_t size,
    size_t alignment,
    void** pptr
    );

ze_result_t __zecall
zeDriverAllocDeviceMem(
    ze_driver_handle_t hDriver,
    ze_device_handle_t hDevice,
    ze_device_mem_alloc_flag_t flags,
    uint32_t ordinal,
    size_t size,
    size_t alignment,
    void** pptr
    );

ze_result_t __zecall
zeDriverAllocHostMem(
    ze_driver_handle_t hDriver,
    ze_host_mem_alloc_flag_t flags,
    size_t size,
    size_t alignment,
    void** pptr
    );

ze_result_t __zecall
zeDriverFreeMem(
    ze_driver_handle_t hDriver,
    void* ptr
    );

typedef enum _ze_memory_allocation_properties_version_t
{
    ZE_MEMORY_ALLOCATION_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_memory_allocation_properties_version_t;

typedef enum _ze_memory_type_t
{
    ZE_MEMORY_TYPE_UNKNOWN = 0,
    ZE_MEMORY_TYPE_HOST,
    ZE_MEMORY_TYPE_DEVICE,
    ZE_MEMORY_TYPE_SHARED,

} ze_memory_type_t;

typedef struct _ze_memory_allocation_properties_t
{
    ze_memory_allocation_properties_version_t version;
    ze_memory_type_t type;
    uint64_t id;

} ze_memory_allocation_properties_t;

ze_result_t __zecall
zeDriverGetMemAllocProperties(
    ze_driver_handle_t hDriver,
    const void* ptr,
    ze_memory_allocation_properties_t* pMemAllocProperties,
    ze_device_handle_t* phDevice
    );

ze_result_t __zecall
zeDriverGetMemAddressRange(
    ze_driver_handle_t hDriver,
    const void* ptr,
    void** pBase,
    size_t* pSize
    );

ze_result_t __zecall
zeDriverGetMemIpcHandle(
    ze_driver_handle_t hDriver,
    const void* ptr,
    ze_ipc_mem_handle_t* pIpcHandle
    );

typedef enum _ze_ipc_memory_flag_t
{
    ZE_IPC_MEMORY_FLAG_NONE = 0,

} ze_ipc_memory_flag_t;

ze_result_t __zecall
zeDriverOpenMemIpcHandle(
    ze_driver_handle_t hDriver,
    ze_device_handle_t hDevice,
    ze_ipc_mem_handle_t handle,
    ze_ipc_memory_flag_t flags,
    void** pptr
    );

ze_result_t __zecall
zeDriverCloseMemIpcHandle(
    ze_driver_handle_t hDriver,
    const void* ptr
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_MEMORY_H
