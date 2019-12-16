/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_sampler.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Sampler
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/sampler.yml
 * @endcond
 *
 */
#ifndef _ZE_SAMPLER_H
#define _ZE_SAMPLER_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_sampler_desc_version_t
{
    ZE_SAMPLER_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_sampler_desc_version_t;

typedef enum _ze_sampler_address_mode_t
{
    ZE_SAMPLER_ADDRESS_MODE_NONE = 0,
    ZE_SAMPLER_ADDRESS_MODE_REPEAT,
    ZE_SAMPLER_ADDRESS_MODE_CLAMP,
    ZE_SAMPLER_ADDRESS_MODE_MIRROR,

} ze_sampler_address_mode_t;

typedef enum _ze_sampler_filter_mode_t
{
    ZE_SAMPLER_FILTER_MODE_NEAREST = 0,
    ZE_SAMPLER_FILTER_MODE_LINEAR,

} ze_sampler_filter_mode_t;

typedef struct _ze_sampler_desc_t
{
    ze_sampler_desc_version_t version;
    ze_sampler_address_mode_t addressMode;
    ze_sampler_filter_mode_t filterMode;
    ze_bool_t isNormalized;

} ze_sampler_desc_t;

ze_result_t __zecall
zeSamplerCreate(
    ze_device_handle_t hDevice,
    const ze_sampler_desc_t* desc,
    ze_sampler_handle_t* phSampler
    );

ze_result_t __zecall
zeSamplerDestroy(
    ze_sampler_handle_t hSampler
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_SAMPLER_H
