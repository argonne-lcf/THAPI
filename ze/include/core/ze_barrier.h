/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_barrier.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Barrier
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/barrier.yml
 * @endcond
 *
 */
#ifndef _ZE_BARRIER_H
#define _ZE_BARRIER_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ze_result_t __zecall
zeCommandListAppendBarrier(
    ze_command_list_handle_t hCommandList,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

ze_result_t __zecall
zeCommandListAppendMemoryRangesBarrier(
    ze_command_list_handle_t hCommandList,
    uint32_t numRanges,
    const size_t* pRangeSizes,
    const void** pRanges,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents
    );

ze_result_t __zecall
zeDeviceSystemBarrier(
    ze_device_handle_t hDevice
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_BARRIER_H
