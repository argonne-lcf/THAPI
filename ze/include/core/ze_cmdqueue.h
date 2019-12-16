/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_cmdqueue.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Command Queue
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/cmdqueue.yml
 * @endcond
 *
 */
#ifndef _ZE_CMDQUEUE_H
#define _ZE_CMDQUEUE_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_command_queue_desc_version_t
{
    ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_command_queue_desc_version_t;

typedef enum _ze_command_queue_flag_t
{
    ZE_COMMAND_QUEUE_FLAG_NONE = 0,
    ZE_COMMAND_QUEUE_FLAG_COPY_ONLY = ZE_BIT(0),
    ZE_COMMAND_QUEUE_FLAG_LOGICAL_ONLY = ZE_BIT(1),
    ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY = ZE_BIT(2),
    ZE_COMMAND_QUEUE_FLAG_SUPPORTS_COOPERATIVE_KERNELS = ZE_BIT(3),

} ze_command_queue_flag_t;

typedef enum _ze_command_queue_mode_t
{
    ZE_COMMAND_QUEUE_MODE_DEFAULT = 0,
    ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
    ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,

} ze_command_queue_mode_t;

typedef enum _ze_command_queue_priority_t
{
    ZE_COMMAND_QUEUE_PRIORITY_NORMAL = 0,
    ZE_COMMAND_QUEUE_PRIORITY_LOW,
    ZE_COMMAND_QUEUE_PRIORITY_HIGH,

} ze_command_queue_priority_t;

typedef struct _ze_command_queue_desc_t
{
    ze_command_queue_desc_version_t version;
    ze_command_queue_flag_t flags;
    ze_command_queue_mode_t mode;
    ze_command_queue_priority_t priority;
    uint32_t ordinal;

} ze_command_queue_desc_t;

ze_result_t __zecall
zeCommandQueueCreate(
    ze_device_handle_t hDevice,
    const ze_command_queue_desc_t* desc,
    ze_command_queue_handle_t* phCommandQueue
    );

ze_result_t __zecall
zeCommandQueueDestroy(
    ze_command_queue_handle_t hCommandQueue
    );

ze_result_t __zecall
zeCommandQueueExecuteCommandLists(
    ze_command_queue_handle_t hCommandQueue,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    ze_fence_handle_t hFence
    );

ze_result_t __zecall
zeCommandQueueSynchronize(
    ze_command_queue_handle_t hCommandQueue,
    uint32_t timeout
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_CMDQUEUE_H
