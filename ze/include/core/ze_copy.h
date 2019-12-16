/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_copy.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Copies
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/copy.yml
 * @endcond
 *
 */
#ifndef _ZE_COPY_H
#define _ZE_COPY_H
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
zeCommandListAppendMemoryCopy(
    ze_command_list_handle_t hCommandList,
    void* dstptr,
    const void* srcptr,
    size_t size,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendMemorySet(
    ze_command_list_handle_t hCommandList,
    void* ptr,
    int value,
    size_t size,
    ze_event_handle_t hEvent
    );

typedef struct _ze_copy_region_t
{
    uint32_t originX;
    uint32_t originY;
    uint32_t width;
    uint32_t height;

} ze_copy_region_t;

ze_result_t __zecall
zeCommandListAppendMemoryCopyRegion(
    ze_command_list_handle_t hCommandList,
    void* dstptr,
    const ze_copy_region_t* dstRegion,
    uint32_t dstPitch,
    const void* srcptr,
    const ze_copy_region_t* srcRegion,
    uint32_t srcPitch,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendImageCopy(
    ze_command_list_handle_t hCommandList,
    ze_image_handle_t hDstImage,
    ze_image_handle_t hSrcImage,
    ze_event_handle_t hEvent
    );

typedef struct _ze_image_region_t
{
    uint32_t originX;
    uint32_t originY;
    uint32_t originZ;
    uint32_t width;
    uint32_t height;
    uint32_t depth;

} ze_image_region_t;

ze_result_t __zecall
zeCommandListAppendImageCopyRegion(
    ze_command_list_handle_t hCommandList,
    ze_image_handle_t hDstImage,
    ze_image_handle_t hSrcImage,
    const ze_image_region_t* pDstRegion,
    const ze_image_region_t* pSrcRegion,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendImageCopyToMemory(
    ze_command_list_handle_t hCommandList,
    void* dstptr,
    ze_image_handle_t hSrcImage,
    const ze_image_region_t* pSrcRegion,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendImageCopyFromMemory(
    ze_command_list_handle_t hCommandList,
    ze_image_handle_t hDstImage,
    const void* srcptr,
    const ze_image_region_t* pDstRegion,
    ze_event_handle_t hEvent
    );

ze_result_t __zecall
zeCommandListAppendMemoryPrefetch(
    ze_command_list_handle_t hCommandList,
    const void* ptr,
    size_t size
    );

typedef enum _ze_memory_advice_t
{
    ZE_MEMORY_ADVICE_SET_READ_MOSTLY = 0,
    ZE_MEMORY_ADVICE_CLEAR_READ_MOSTLY,
    ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION,
    ZE_MEMORY_ADVICE_CLEAR_PREFERRED_LOCATION,
    ZE_MEMORY_ADVICE_SET_ACCESSED_BY,
    ZE_MEMORY_ADVICE_CLEAR_ACCESSED_BY,
    ZE_MEMORY_ADVICE_SET_NON_ATOMIC_MOSTLY,
    ZE_MEMORY_ADVICE_CLEAR_NON_ATOMIC_MOSTLY,
    ZE_MEMORY_ADVICE_BIAS_CACHED,
    ZE_MEMORY_ADVICE_BIAS_UNCACHED,

} ze_memory_advice_t;

ze_result_t __zecall
zeCommandListAppendMemAdvise(
    ze_command_list_handle_t hCommandList,
    ze_device_handle_t hDevice,
    const void* ptr,
    size_t size,
    ze_memory_advice_t advice
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_COPY_H
