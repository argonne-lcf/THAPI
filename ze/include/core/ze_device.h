/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_device.h
 *
 * @brief Intel 'One API' Level-Zero APIs for Device
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/device.yml
 * @endcond
 *
 */
#ifndef _ZE_DEVICE_H
#define _ZE_DEVICE_H
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
zeDeviceGet(
    ze_driver_handle_t hDriver,
    uint32_t* pCount,
    ze_device_handle_t* phDevices
    );

ze_result_t __zecall
zeDeviceGetSubDevices(
    ze_device_handle_t hDevice,
    uint32_t* pCount,
    ze_device_handle_t* phSubdevices
    );

typedef enum _ze_device_properties_version_t
{
    ZE_DEVICE_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_properties_version_t;

#ifndef ZE_MAX_UUID_SIZE
#define ZE_MAX_UUID_SIZE  16
#endif // ZE_MAX_UUID_SIZE

typedef enum _ze_device_type_t
{
    ZE_DEVICE_TYPE_GPU = 1,
    ZE_DEVICE_TYPE_FPGA,

} ze_device_type_t;

typedef struct _ze_device_uuid_t
{
    uint8_t id[ZE_MAX_UUID_SIZE];

} ze_device_uuid_t;

#ifndef ZE_MAX_DEVICE_NAME
#define ZE_MAX_DEVICE_NAME  256
#endif // ZE_MAX_DEVICE_NAME

typedef struct _ze_device_properties_t
{
    ze_device_properties_version_t version;
    ze_device_type_t type;
    uint32_t vendorId;
    uint32_t deviceId;
    ze_device_uuid_t uuid;
    ze_bool_t isSubdevice;
    uint32_t subdeviceId;
    uint32_t coreClockRate;
    ze_bool_t unifiedMemorySupported;
    ze_bool_t onDemandPageFaultsSupported;
    uint32_t maxCommandQueues;
    uint32_t numAsyncComputeEngines;
    uint32_t numAsyncCopyEngines;
    uint32_t maxCommandQueuePriority;
    uint32_t numThreadsPerEU;
    uint32_t physicalEUSimdWidth;
    uint32_t numEUsPerSubslice;
    uint32_t numSubslicesPerSlice;
    uint32_t numSlicesPerTile;
    uint32_t numTiles;
    char name[ZE_MAX_DEVICE_NAME];

} ze_device_properties_t;

ze_result_t __zecall
zeDeviceGetProperties(
    ze_device_handle_t hDevice,
    ze_device_properties_t* pDeviceProperties
    );

typedef enum _ze_device_compute_properties_version_t
{
    ZE_DEVICE_COMPUTE_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_compute_properties_version_t;

#ifndef ZE_SUBGROUPSIZE_COUNT
#define ZE_SUBGROUPSIZE_COUNT  8
#endif // ZE_SUBGROUPSIZE_COUNT

typedef struct _ze_device_compute_properties_t
{
    ze_device_compute_properties_version_t version;
    uint32_t maxTotalGroupSize;
    uint32_t maxGroupSizeX;
    uint32_t maxGroupSizeY;
    uint32_t maxGroupSizeZ;
    uint32_t maxGroupCountX;
    uint32_t maxGroupCountY;
    uint32_t maxGroupCountZ;
    uint32_t maxSharedLocalMemory;
    uint32_t numSubGroupSizes;
    uint32_t subGroupSizes[ZE_SUBGROUPSIZE_COUNT];

} ze_device_compute_properties_t;

ze_result_t __zecall
zeDeviceGetComputeProperties(
    ze_device_handle_t hDevice,
    ze_device_compute_properties_t* pComputeProperties
    );

typedef enum _ze_device_memory_properties_version_t
{
    ZE_DEVICE_MEMORY_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_memory_properties_version_t;

typedef struct _ze_device_memory_properties_t
{
    ze_device_memory_properties_version_t version;
    uint32_t maxClockRate;
    uint32_t maxBusWidth;
    uint64_t totalSize;

} ze_device_memory_properties_t;

ze_result_t __zecall
zeDeviceGetMemoryProperties(
    ze_device_handle_t hDevice,
    uint32_t* pCount,
    ze_device_memory_properties_t* pMemProperties
    );

typedef enum _ze_device_memory_access_properties_version_t
{
    ZE_DEVICE_MEMORY_ACCESS_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_memory_access_properties_version_t;

typedef enum _ze_memory_access_capabilities_t
{
    ZE_MEMORY_ACCESS_NONE = 0,
    ZE_MEMORY_ACCESS = ZE_BIT( 0 ),
    ZE_MEMORY_ATOMIC_ACCESS = ZE_BIT( 1 ),
    ZE_MEMORY_CONCURRENT_ACCESS = ZE_BIT( 2 ),
    ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS = ZE_BIT( 3 ),

} ze_memory_access_capabilities_t;

typedef struct _ze_device_memory_access_properties_t
{
    ze_device_memory_access_properties_version_t version;
    ze_memory_access_capabilities_t hostAllocCapabilities;
    ze_memory_access_capabilities_t deviceAllocCapabilities;
    ze_memory_access_capabilities_t sharedSingleDeviceAllocCapabilities;
    ze_memory_access_capabilities_t sharedCrossDeviceAllocCapabilities;
    ze_memory_access_capabilities_t sharedSystemAllocCapabilities;

} ze_device_memory_access_properties_t;

ze_result_t __zecall
zeDeviceGetMemoryAccessProperties(
    ze_device_handle_t hDevice,
    ze_device_memory_access_properties_t* pMemAccessProperties
    );

typedef enum _ze_device_cache_properties_version_t
{
    ZE_DEVICE_CACHE_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_cache_properties_version_t;

typedef struct _ze_device_cache_properties_t
{
    ze_device_cache_properties_version_t version;
    ze_bool_t intermediateCacheControlSupported;
    size_t intermediateCacheSize;
    ze_bool_t lastLevelCacheSizeControlSupported;
    size_t lastLevelCacheSize;

} ze_device_cache_properties_t;

ze_result_t __zecall
zeDeviceGetCacheProperties(
    ze_device_handle_t hDevice,
    ze_device_cache_properties_t* pCacheProperties
    );

typedef enum _ze_device_image_properties_version_t
{
    ZE_DEVICE_IMAGE_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_image_properties_version_t;

typedef struct _ze_device_image_properties_t
{
    ze_device_image_properties_version_t version;
    ze_bool_t supported;
    uint32_t maxImageDims1D;
    uint32_t maxImageDims2D;
    uint32_t maxImageDims3D;
    uint32_t maxImageArraySlices;

} ze_device_image_properties_t;

ze_result_t __zecall
zeDeviceGetImageProperties(
    ze_device_handle_t hDevice,
    ze_device_image_properties_t* pImageProperties
    );

typedef enum _ze_device_p2p_properties_version_t
{
    ZE_DEVICE_P2P_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} ze_device_p2p_properties_version_t;

typedef struct _ze_device_p2p_properties_t
{
    ze_device_p2p_properties_version_t version;
    ze_bool_t accessSupported;
    ze_bool_t atomicsSupported;

} ze_device_p2p_properties_t;

ze_result_t __zecall
zeDeviceGetP2PProperties(
    ze_device_handle_t hDevice,
    ze_device_handle_t hPeerDevice,
    ze_device_p2p_properties_t* pP2PProperties
    );

ze_result_t __zecall
zeDeviceCanAccessPeer(
    ze_device_handle_t hDevice,
    ze_device_handle_t hPeerDevice,
    ze_bool_t* value
    );

typedef enum _ze_cache_config_t
{
    ZE_CACHE_CONFIG_DEFAULT = ZE_BIT( 0 ),
    ZE_CACHE_CONFIG_LARGE_SLM = ZE_BIT( 1 ),
    ZE_CACHE_CONFIG_LARGE_DATA = ZE_BIT( 2 ),

} ze_cache_config_t;

ze_result_t __zecall
zeDeviceSetLastLevelCacheConfig(
    ze_device_handle_t hDevice,
    ze_cache_config_t CacheConfig
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_DEVICE_H
