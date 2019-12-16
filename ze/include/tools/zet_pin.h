/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_pin.h
 *
 * @brief Intel 'One API' Level-Zero Tool APIs for Program Instrumentation (PIN)
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/pin.yml
 * @endcond
 *
 */
#ifndef _ZET_PIN_H
#define _ZET_PIN_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ze_result_t __zecall
zetModuleGetKernelNames(
    zet_module_handle_t hModule,
    uint32_t* pCount,
    const char** pNames
    );

typedef enum _zet_profile_info_version_t
{
    ZET_PROFILE_INFO_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_profile_info_version_t;

typedef enum _zet_profile_flag_t
{
    ZET_PROFILE_FLAG_REGISTER_REALLOCATION = ZE_BIT(0),
    ZET_PROFILE_FLAG_FREE_REGISTER_INFO = ZE_BIT(1),

} zet_profile_flag_t;

typedef struct _zet_profile_info_t
{
    zet_profile_info_version_t version;
    zet_profile_flag_t flags;
    uint32_t numTokens;

} zet_profile_info_t;

typedef enum _zet_profile_token_type_t
{
    ZET_PROFILE_TOKEN_TYPE_FREE_REGISTER,

} zet_profile_token_type_t;

typedef struct _zet_profile_free_register_token_t
{
    zet_profile_token_type_t type;
    uint32_t size;
    uint32_t count;

} zet_profile_free_register_token_t;

typedef struct _zet_profile_register_sequence_t
{
    uint32_t start;
    uint32_t count;

} zet_profile_register_sequence_t;

ze_result_t __zecall
zetKernelGetProfileInfo(
    zet_kernel_handle_t hKernel,
    zet_profile_info_t* pInfo
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_PIN_H
