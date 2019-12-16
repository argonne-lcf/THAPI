/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_event.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Event
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/event.yml
 * @endcond
 *
 */
#ifndef _ZE_EVENT_H
#define _ZE_EVENT_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_event_pool_desc_version_t
{
    ZE_EVENT_POOL_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_event_pool_desc_version_t;

typedef enum _ze_event_pool_flag_t
{
    ZE_EVENT_POOL_FLAG_DEFAULT = 0,
    ZE_EVENT_POOL_FLAG_HOST_VISIBLE = ZE_BIT(0),
    ZE_EVENT_POOL_FLAG_IPC = ZE_BIT(1),

} ze_event_pool_flag_t;

typedef struct _ze_event_pool_desc_t
{
    ze_event_pool_desc_version_t version;
    ze_event_pool_flag_t flags;
    uint32_t count;

} ze_event_pool_desc_t;

ze_result_t __zecall
zeEventPoolCreate(
    ze_driver_handle_t hDriver,
    const ze_event_pool_desc_t* desc,
    uint32_t numDevices,
    ze_device_handle_t* phDevices,
    ze_event_pool_handle_t* phEventPool
    );

ze_result_t __zecall
zeEventPoolDestroy(
    ze_event_pool_handle_t hEventPool
    );

typedef enum _ze_event_desc_version_t
{
    ZE_EVENT_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_event_desc_version_t;

typedef enum _ze_event_scope_flag_t
{
    ZE_EVENT_SCOPE_FLAG_NONE = 0,
    ZE_EVENT_SCOPE_FLAG_SUBDEVICE = ZE_BIT(0),
    ZE_EVENT_SCOPE_FLAG_DEVICE = ZE_BIT(1),
    ZE_EVENT_SCOPE_FLAG_HOST = ZE_BIT(2),

} ze_event_scope_flag_t;

typedef struct _ze_event_desc_t
{
    ze_event_desc_version_t version;
    uint32_t index;
    ze_event_scope_flag_t signal;
    ze_event_scope_flag_t wait;

} ze_event_desc_t;

ze_result_t __zecall
zeEventCreate(
    ze_event_pool_handle_t hEventPool,
    const ze_event_desc_t* desc,
    ze_event_handle_t* phEvent
    );

ze_result_t __zecall
zeEventDestroy(
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeEventPoolGetIpcHandle(
    ze_event_pool_handle_t hEventPool,
    ze_ipc_event_pool_handle_t* phIpc
    );

ze_result_t __zecall
zeEventPoolOpenIpcHandle(
    ze_driver_handle_t hDriver,
    ze_ipc_event_pool_handle_t hIpc,
    ze_event_pool_handle_t* phEventPool
    );

ze_result_t __zecall
zeEventPoolCloseIpcHandle(
    ze_event_pool_handle_t hEventPool
    );

ze_result_t __zecall
zeCommandListAppendSignalEvent(
    ze_command_list_handle_t hCommandList,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendWaitOnEvents(
    ze_command_list_handle_t hCommandList,
    uint32_t numEvents,
    ze_event_handle_t* phEvents
    );

ze_result_t __zecall
zeEventHostSignal(
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeEventHostSynchronize(
    ze_event_handle_t hEvent,
    uint32_t timeout
    );

ze_result_t __zecall
zeEventQueryStatus(
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendEventReset(
    ze_command_list_handle_t hCommandList,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeEventReset(
    ze_event_handle_t hEvent
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_EVENT_H
