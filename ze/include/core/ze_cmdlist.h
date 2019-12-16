/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_cmdlist.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Command List
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/cmdlist.yml
 * @endcond
 *
 */
#ifndef _ZE_CMDLIST_H
#define _ZE_CMDLIST_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_command_list_desc_version_t
{
    ZE_COMMAND_LIST_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_command_list_desc_version_t;

typedef enum _ze_command_list_flag_t
{
    ZE_COMMAND_LIST_FLAG_NONE = 0,
    ZE_COMMAND_LIST_FLAG_COPY_ONLY = ZE_BIT(0),
    ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING = ZE_BIT(1),
    ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT = ZE_BIT(2),
    ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY = ZE_BIT(3),

} ze_command_list_flag_t;

typedef struct _ze_command_list_desc_t
{
    ze_command_list_desc_version_t version;
    ze_command_list_flag_t flags;

} ze_command_list_desc_t;

ze_result_t __zecall
zeCommandListCreate(
    ze_device_handle_t hDevice,
    const ze_command_list_desc_t* desc,
    ze_command_list_handle_t* phCommandList
    );

ze_result_t __zecall
zeCommandListCreateImmediate(
    ze_device_handle_t hDevice,
    const ze_command_queue_desc_t* altdesc,
    ze_command_list_handle_t* phCommandList
    );

ze_result_t __zecall
zeCommandListDestroy(
    ze_command_list_handle_t hCommandList
    );

ze_result_t __zecall
zeCommandListClose(
    ze_command_list_handle_t hCommandList
    );

ze_result_t __zecall
zeCommandListReset(
    ze_command_list_handle_t hCommandList
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_CMDLIST_H
