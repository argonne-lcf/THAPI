/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_driver.h
 *
 * @brief Intel 'One API' Level-Zero APIs
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/driver.yml
 * @endcond
 *
 */
#ifndef _ZE_DRIVER_H
#define _ZE_DRIVER_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_init_flag_t
{
    ZE_INIT_FLAG_NONE = 0,
    ZE_INIT_FLAG_GPU_ONLY = ZE_BIT(0),

} ze_init_flag_t;

ze_result_t __zecall
zeInit(
    ze_init_flag_t flags
    );

ze_result_t __zecall
zeDriverGet(
    uint32_t* pCount,
    ze_driver_handle_t* phDrivers
    );

ze_result_t __zecall
zeDriverGetDriverVersion(
    ze_driver_handle_t hDriver,
    uint32_t* version
    );

typedef enum _ze_api_version_t
{
    ZE_API_VERSION_1_0 = ZE_MAKE_VERSION( 1, 0 ),

} ze_api_version_t;

ze_result_t __zecall
zeDriverGetApiVersion(
    ze_driver_handle_t hDrivers,
    ze_api_version_t* version
    );

typedef enum _ze_driver_ipc_properties_version_t
{
    ZE_DRIVER_IPC_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_driver_ipc_properties_version_t;

typedef struct _ze_driver_ipc_properties_t
{
    ze_driver_ipc_properties_version_t version;
    ze_bool_t memsSupported;
    ze_bool_t eventsSupported;

} ze_driver_ipc_properties_t;

ze_result_t __zecall
zeDriverGetIPCProperties(
    ze_driver_handle_t hDriver,
    ze_driver_ipc_properties_t* pIPCProperties
    );

ze_result_t __zecall
zeDriverGetExtensionFunctionAddress(
    ze_driver_handle_t hDriver,
    const char* pFuncName,
    void** pfunc
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_DRIVER_H
