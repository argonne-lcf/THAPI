/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_common.h
 *
 * @brief Intel 'One API' Level-Zero API common types
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/common.yml
 * @endcond
 *
 */
#ifndef _ZE_COMMON_H
#define _ZE_COMMON_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef ZE_MAKE_VERSION
#define ZE_MAKE_VERSION( _major, _minor )  (( _major << 16 )|( _minor & 0x0000ffff))
#endif // ZE_MAKE_VERSION

#ifndef ZE_MAJOR_VERSION
#define ZE_MAJOR_VERSION( _ver )  ( _ver >> 16 )
#endif // ZE_MAJOR_VERSION

#ifndef ZE_MINOR_VERSION
#define ZE_MINOR_VERSION( _ver )  ( _ver & 0x0000ffff )
#endif // ZE_MINOR_VERSION

#ifndef __zecall
#if defined(_WIN32)
#define __zecall  __cdecl
#else
#define __zecall
#endif // defined(_WIN32)
#endif // __zecall

#ifndef __zedllexport
#if defined(_WIN32)
#define __zedllexport  __declspec(dllexport)
#else
#define __zedllexport
#endif // defined(_WIN32)
#endif // __zedllexport

#ifndef ZE_ENABLE_OCL_INTEROP
#if !defined(ZE_ENABLE_OCL_INTEROP)
#define ZE_ENABLE_OCL_INTEROP  0
#endif // !defined(ZE_ENABLE_OCL_INTEROP)
#endif // ZE_ENABLE_OCL_INTEROP

typedef uint8_t ze_bool_t;

typedef struct _ze_driver_handle_t *ze_driver_handle_t;

typedef struct _ze_device_handle_t *ze_device_handle_t;

typedef struct _ze_command_queue_handle_t *ze_command_queue_handle_t;

typedef struct _ze_command_list_handle_t *ze_command_list_handle_t;

typedef struct _ze_fence_handle_t *ze_fence_handle_t;

typedef struct _ze_event_pool_handle_t *ze_event_pool_handle_t;

typedef struct _ze_event_handle_t *ze_event_handle_t;

typedef struct _ze_image_handle_t *ze_image_handle_t;

typedef struct _ze_module_handle_t *ze_module_handle_t;

typedef struct _ze_module_build_log_handle_t *ze_module_build_log_handle_t;

typedef struct _ze_kernel_handle_t *ze_kernel_handle_t;

typedef struct _ze_sampler_handle_t *ze_sampler_handle_t;

#ifndef ZE_MAX_IPC_HANDLE_SIZE
#define ZE_MAX_IPC_HANDLE_SIZE  64
#endif // ZE_MAX_IPC_HANDLE_SIZE

typedef struct _ze_ipc_mem_handle_t
{
    char data[ZE_MAX_IPC_HANDLE_SIZE];

} ze_ipc_mem_handle_t;

typedef struct _ze_ipc_event_pool_handle_t
{
    char data[ZE_MAX_IPC_HANDLE_SIZE];

} ze_ipc_event_pool_handle_t;

#ifndef ZE_BIT
#define ZE_BIT( _i )  ( 1 << _i )
#endif // ZE_BIT

typedef enum _ze_result_t
{
    ZE_RESULT_SUCCESS = 0,
    ZE_RESULT_NOT_READY = 1,
    ZE_RESULT_ERROR_UNINITIALIZED,
    ZE_RESULT_ERROR_DEVICE_LOST,
    ZE_RESULT_ERROR_UNSUPPORTED,
    ZE_RESULT_ERROR_INVALID_ARGUMENT,
    ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY,
    ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY,
    ZE_RESULT_ERROR_MODULE_BUILD_FAILURE,
    ZE_RESULT_ERROR_INSUFFICENT_PERMISSIONS,
    ZE_RESULT_ERROR_DEVICE_IS_IN_USE,
    ZE_RESULT_ERROR_ARRAY_SIZE_TOO_SMALL,
    ZE_RESULT_ERROR_DEVICE_ACCESS,
    ZE_RESULT_ERROR_FEATURE_LOCKED,
    ZE_RESULT_ERROR_UNKNOWN = 0x7fffffff,

} ze_result_t;

//typedef struct _ze_ipc_mem_handle_t ze_ipc_mem_handle_t;
//
//typedef struct _ze_ipc_event_pool_handle_t ze_ipc_event_pool_handle_t;
//
//typedef struct _ze_driver_ipc_properties_t ze_driver_ipc_properties_t;
//
//typedef struct _ze_device_uuid_t ze_device_uuid_t;
//
//typedef struct _ze_device_properties_t ze_device_properties_t;
//
//typedef struct _ze_device_compute_properties_t ze_device_compute_properties_t;
//
//typedef struct _ze_device_memory_properties_t ze_device_memory_properties_t;
//
//typedef struct _ze_device_memory_access_properties_t ze_device_memory_access_properties_t;
//
//typedef struct _ze_device_cache_properties_t ze_device_cache_properties_t;
//
//typedef struct _ze_device_image_properties_t ze_device_image_properties_t;
//
//typedef struct _ze_device_p2p_properties_t ze_device_p2p_properties_t;
//
//typedef struct _ze_command_queue_desc_t ze_command_queue_desc_t;
//
//typedef struct _ze_command_list_desc_t ze_command_list_desc_t;
//
//typedef struct _ze_copy_region_t ze_copy_region_t;
//
//typedef struct _ze_image_region_t ze_image_region_t;
//
//typedef struct _ze_event_pool_desc_t ze_event_pool_desc_t;
//
//typedef struct _ze_event_desc_t ze_event_desc_t;
//
//typedef struct _ze_fence_desc_t ze_fence_desc_t;
//
//typedef struct _ze_image_format_desc_t ze_image_format_desc_t;
//
//typedef struct _ze_image_desc_t ze_image_desc_t;
//
//typedef struct _ze_image_properties_t ze_image_properties_t;
//
//typedef struct _ze_memory_allocation_properties_t ze_memory_allocation_properties_t;
//
//typedef struct _ze_module_constants_t ze_module_constants_t;
//
//typedef struct _ze_module_desc_t ze_module_desc_t;
//
//typedef struct _ze_kernel_desc_t ze_kernel_desc_t;
//
//typedef struct _ze_thread_group_dimensions_t ze_thread_group_dimensions_t;
//
//typedef struct _ze_kernel_properties_t ze_kernel_properties_t;
//
//typedef struct _ze_sampler_desc_t ze_sampler_desc_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_COMMON_H
