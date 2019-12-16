/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_image.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Images
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/image.yml
 * @endcond
 *
 */
#ifndef _ZE_IMAGE_H
#define _ZE_IMAGE_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ze_image_desc_version_t
{
    ZE_IMAGE_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_image_desc_version_t;

typedef enum _ze_image_flag_t
{
    ZE_IMAGE_FLAG_PROGRAM_READ = ZE_BIT( 0 ),
    ZE_IMAGE_FLAG_PROGRAM_WRITE = ZE_BIT( 1 ),
    ZE_IMAGE_FLAG_BIAS_CACHED = ZE_BIT( 2 ),
    ZE_IMAGE_FLAG_BIAS_UNCACHED = ZE_BIT( 3 ),

} ze_image_flag_t;

typedef enum _ze_image_type_t
{
    ZE_IMAGE_TYPE_1D,
    ZE_IMAGE_TYPE_1DARRAY,
    ZE_IMAGE_TYPE_2D,
    ZE_IMAGE_TYPE_2DARRAY,
    ZE_IMAGE_TYPE_3D,

} ze_image_type_t;

typedef enum _ze_image_format_layout_t
{
    ZE_IMAGE_FORMAT_LAYOUT_8,
    ZE_IMAGE_FORMAT_LAYOUT_16,
    ZE_IMAGE_FORMAT_LAYOUT_32,
    ZE_IMAGE_FORMAT_LAYOUT_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
    ZE_IMAGE_FORMAT_LAYOUT_11_11_10,
    ZE_IMAGE_FORMAT_LAYOUT_5_6_5,
    ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
    ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4,
    ZE_IMAGE_FORMAT_LAYOUT_Y8,
    ZE_IMAGE_FORMAT_LAYOUT_NV12,
    ZE_IMAGE_FORMAT_LAYOUT_YUYV,
    ZE_IMAGE_FORMAT_LAYOUT_VYUY,
    ZE_IMAGE_FORMAT_LAYOUT_YVYU,
    ZE_IMAGE_FORMAT_LAYOUT_UYVY,
    ZE_IMAGE_FORMAT_LAYOUT_AYUV,
    ZE_IMAGE_FORMAT_LAYOUT_YUAV,
    ZE_IMAGE_FORMAT_LAYOUT_P010,
    ZE_IMAGE_FORMAT_LAYOUT_Y410,
    ZE_IMAGE_FORMAT_LAYOUT_P012,
    ZE_IMAGE_FORMAT_LAYOUT_Y16,
    ZE_IMAGE_FORMAT_LAYOUT_P016,
    ZE_IMAGE_FORMAT_LAYOUT_Y216,
    ZE_IMAGE_FORMAT_LAYOUT_P216,
    ZE_IMAGE_FORMAT_LAYOUT_P416,

} ze_image_format_layout_t;

typedef enum _ze_image_format_type_t
{
    ZE_IMAGE_FORMAT_TYPE_UINT,
    ZE_IMAGE_FORMAT_TYPE_SINT,
    ZE_IMAGE_FORMAT_TYPE_UNORM,
    ZE_IMAGE_FORMAT_TYPE_SNORM,
    ZE_IMAGE_FORMAT_TYPE_FLOAT,

} ze_image_format_type_t;

typedef enum _ze_image_format_swizzle_t
{
    ZE_IMAGE_FORMAT_SWIZZLE_R,
    ZE_IMAGE_FORMAT_SWIZZLE_G,
    ZE_IMAGE_FORMAT_SWIZZLE_B,
    ZE_IMAGE_FORMAT_SWIZZLE_A,
    ZE_IMAGE_FORMAT_SWIZZLE_0,
    ZE_IMAGE_FORMAT_SWIZZLE_1,
    ZE_IMAGE_FORMAT_SWIZZLE_X,

} ze_image_format_swizzle_t;

typedef struct _ze_image_format_desc_t
{
    ze_image_format_layout_t layout;
    ze_image_format_type_t type;
    ze_image_format_swizzle_t x;
    ze_image_format_swizzle_t y;
    ze_image_format_swizzle_t z;
    ze_image_format_swizzle_t w;

} ze_image_format_desc_t;

typedef struct _ze_image_desc_t
{
    ze_image_desc_version_t version;
    ze_image_flag_t flags;
    ze_image_type_t type;
    ze_image_format_desc_t format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arraylevels;
    uint32_t miplevels;

} ze_image_desc_t;

typedef enum _ze_image_properties_version_t
{
    ZE_IMAGE_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_image_properties_version_t;

typedef enum _ze_image_sampler_filter_flags_t
{
    ZE_IMAGE_SAMPLER_FILTER_FLAGS_NONE = 0,
    ZE_IMAGE_SAMPLER_FILTER_FLAGS_POINT = ZE_BIT(0),
    ZE_IMAGE_SAMPLER_FILTER_FLAGS_LINEAR = ZE_BIT(1),

} ze_image_sampler_filter_flags_t;

typedef struct _ze_image_properties_t
{
    ze_image_properties_version_t version;
    ze_image_sampler_filter_flags_t samplerFilterFlags;

} ze_image_properties_t;

ze_result_t __zecall
zeImageGetProperties(
    ze_device_handle_t hDevice,
    const ze_image_desc_t* desc,
    ze_image_properties_t* pImageProperties
    );

ze_result_t __zecall
zeImageCreate(
    ze_device_handle_t hDevice,
    const ze_image_desc_t* desc,
    ze_image_handle_t* phImage
    );

ze_result_t __zecall
zeImageDestroy(
    ze_image_handle_t hImage
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_IMAGE_H
