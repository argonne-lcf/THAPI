/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_residency.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Memory Residency
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/residency.yml
 * @endcond
 *
 */
#ifndef _ZE_RESIDENCY_H
#define _ZE_RESIDENCY_H
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
zeDeviceMakeMemoryResident(
    ze_device_handle_t hDevice,
    void* ptr,
    size_t size
    );

ze_result_t __zecall
zeDeviceEvictMemory(
    ze_device_handle_t hDevice,
    void* ptr,
    size_t size
    );

ze_result_t __zecall
zeDeviceMakeImageResident(
    ze_device_handle_t hDevice,
    ze_image_handle_t hImage
    );

ze_result_t __zecall
zeDeviceEvictImage(
    ze_device_handle_t hDevice,
    ze_image_handle_t hImage
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_RESIDENCY_H
