/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_fence.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Fence
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/fence.yml
 * @endcond
 *
 */
#ifndef _ZE_FENCE_H
#define _ZE_FENCE_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_fence_desc_version_t
{
    ZE_FENCE_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_fence_desc_version_t;

typedef enum _ze_fence_flag_t
{
    ZE_FENCE_FLAG_NONE = 0,

} ze_fence_flag_t;

typedef struct _ze_fence_desc_t
{
    ze_fence_desc_version_t version;
    ze_fence_flag_t flags;

} ze_fence_desc_t;

ze_result_t __zecall
zeFenceCreate(
    ze_command_queue_handle_t hCommandQueue,
    const ze_fence_desc_t* desc,
    ze_fence_handle_t* phFence
    );

ze_result_t __zecall
zeFenceDestroy(
    ze_fence_handle_t hFence
    );

ze_result_t __zecall
zeFenceHostSynchronize(
    ze_fence_handle_t hFence,
    uint32_t timeout
    );

ze_result_t __zecall
zeFenceQueryStatus(
    ze_fence_handle_t hFence
    );

ze_result_t __zecall
zeFenceReset(
    ze_fence_handle_t hFence
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_FENCE_H
