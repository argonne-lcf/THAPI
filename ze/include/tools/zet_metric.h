/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_metric.h
 *
 * @brief Intel 'One API' Level-Zero Tool APIs for Metric
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/metric.yml
 * @endcond
 *
 */
#ifndef _ZET_METRIC_H
#define _ZET_METRIC_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ze_result_t __zecall
zetMetricGroupGet(
    zet_device_handle_t hDevice,
    uint32_t* pCount,
    zet_metric_group_handle_t* phMetricGroups
    );

#ifndef ZET_MAX_METRIC_GROUP_NAME
#define ZET_MAX_METRIC_GROUP_NAME  256
#endif // ZET_MAX_METRIC_GROUP_NAME

#ifndef ZET_MAX_METRIC_GROUP_DESCRIPTION
#define ZET_MAX_METRIC_GROUP_DESCRIPTION  256
#endif // ZET_MAX_METRIC_GROUP_DESCRIPTION

typedef enum _zet_metric_group_sampling_type_t
{
    ZET_METRIC_GROUP_SAMPLING_TYPE_EVENT_BASED = ZE_BIT(0),
    ZET_METRIC_GROUP_SAMPLING_TYPE_TIME_BASED = ZE_BIT(1),

} zet_metric_group_sampling_type_t;

typedef enum _zet_metric_group_properties_version_t
{
    ZET_METRIC_GROUP_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_metric_group_properties_version_t;

typedef struct _zet_metric_group_properties_t
{
    zet_metric_group_properties_version_t version;
    char name[ZET_MAX_METRIC_GROUP_NAME];
    char description[ZET_MAX_METRIC_GROUP_DESCRIPTION];
    zet_metric_group_sampling_type_t samplingType;
    uint32_t domain;
    uint32_t metricCount;

} zet_metric_group_properties_t;

ze_result_t __zecall
zetMetricGroupGetProperties(
    zet_metric_group_handle_t hMetricGroup,
    zet_metric_group_properties_t* pProperties
    );

typedef enum _zet_metric_type_t
{
    ZET_METRIC_TYPE_DURATION,
    ZET_METRIC_TYPE_EVENT,
    ZET_METRIC_TYPE_EVENT_WITH_RANGE,
    ZET_METRIC_TYPE_THROUGHPUT,
    ZET_METRIC_TYPE_TIMESTAMP,
    ZET_METRIC_TYPE_FLAG,
    ZET_METRIC_TYPE_RATIO,
    ZET_METRIC_TYPE_RAW,

} zet_metric_type_t;

typedef enum _zet_value_type_t
{
    ZET_VALUE_TYPE_UINT32,
    ZET_VALUE_TYPE_UINT64,
    ZET_VALUE_TYPE_FLOAT32,
    ZET_VALUE_TYPE_FLOAT64,
    ZET_VALUE_TYPE_BOOL8,

} zet_value_type_t;

typedef union _zet_value_t
{
    uint32_t ui32;
    uint64_t ui64;
    float fp32;
    double fp64;
    ze_bool_t b8;

} zet_value_t;

typedef struct _zet_typed_value_t
{
    zet_value_type_t type;
    zet_value_t value;

} zet_typed_value_t;

ze_result_t __zecall
zetMetricGroupCalculateMetricValues(
    zet_metric_group_handle_t hMetricGroup,
    size_t rawDataSize,
    const uint8_t* pRawData,
    uint32_t* pMetricValueCount,
    zet_typed_value_t* pMetricValues
    );

ze_result_t __zecall
zetMetricGet(
    zet_metric_group_handle_t hMetricGroup,
    uint32_t* pCount,
    zet_metric_handle_t* phMetrics
    );

#ifndef ZET_MAX_METRIC_NAME
#define ZET_MAX_METRIC_NAME  256
#endif // ZET_MAX_METRIC_NAME

#ifndef ZET_MAX_METRIC_DESCRIPTION
#define ZET_MAX_METRIC_DESCRIPTION  256
#endif // ZET_MAX_METRIC_DESCRIPTION

#ifndef ZET_MAX_METRIC_COMPONENT
#define ZET_MAX_METRIC_COMPONENT  256
#endif // ZET_MAX_METRIC_COMPONENT

#ifndef ZET_MAX_METRIC_RESULT_UNITS
#define ZET_MAX_METRIC_RESULT_UNITS  256
#endif // ZET_MAX_METRIC_RESULT_UNITS

typedef enum _zet_metric_properties_version_t
{
    ZET_METRIC_PROPERTIES_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_metric_properties_version_t;

typedef struct _zet_metric_properties_t
{
    zet_metric_properties_version_t version;
    char name[ZET_MAX_METRIC_NAME];
    char description[ZET_MAX_METRIC_DESCRIPTION];
    char component[ZET_MAX_METRIC_COMPONENT];
    uint32_t tierNumber;
    zet_metric_type_t metricType;
    zet_value_type_t resultType;
    char resultUnits[ZET_MAX_METRIC_RESULT_UNITS];

} zet_metric_properties_t;

ze_result_t __zecall
zetMetricGetProperties(
    zet_metric_handle_t hMetric,
    zet_metric_properties_t* pProperties
    );

ze_result_t __zecall
zetDeviceActivateMetricGroups(
    zet_device_handle_t hDevice,
    uint32_t count,
    zet_metric_group_handle_t* phMetricGroups
    );

typedef enum _zet_metric_tracer_desc_version_t
{
    ZET_METRIC_TRACER_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_metric_tracer_desc_version_t;

typedef struct _zet_metric_tracer_desc_t
{
    zet_metric_tracer_desc_version_t version;
    uint32_t notifyEveryNReports;
    uint32_t samplingPeriod;

} zet_metric_tracer_desc_t;

ze_result_t __zecall
zetMetricTracerOpen(
    zet_device_handle_t hDevice,
    zet_metric_group_handle_t hMetricGroup,
    zet_metric_tracer_desc_t* desc,
    ze_event_handle_t hNotificationEvent,
    zet_metric_tracer_handle_t* phMetricTracer
    );

ze_result_t __zecall
zetCommandListAppendMetricTracerMarker(
    zet_command_list_handle_t hCommandList,
    zet_metric_tracer_handle_t hMetricTracer,
    uint32_t value
    );

ze_result_t __zecall
zetMetricTracerClose(
    zet_metric_tracer_handle_t hMetricTracer
    );

ze_result_t __zecall
zetMetricTracerReadData(
    zet_metric_tracer_handle_t hMetricTracer,
    uint32_t maxReportCount,
    size_t* pRawDataSize,
    uint8_t* pRawData
    );

typedef enum _zet_metric_query_pool_desc_version_t
{
    ZET_METRIC_QUERY_POOL_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_metric_query_pool_desc_version_t;

typedef enum _zet_metric_query_pool_flag_t
{
    ZET_METRIC_QUERY_POOL_FLAG_PERFORMANCE,
    ZET_METRIC_QUERY_POOL_FLAG_SKIP_EXECUTION,

} zet_metric_query_pool_flag_t;

typedef struct _zet_metric_query_pool_desc_t
{
    zet_metric_query_pool_desc_version_t version;
    zet_metric_query_pool_flag_t flags;
    uint32_t count;

} zet_metric_query_pool_desc_t;

ze_result_t __zecall
zetMetricQueryPoolCreate(
    zet_device_handle_t hDevice,
    zet_metric_group_handle_t hMetricGroup,
    const zet_metric_query_pool_desc_t* desc,
    zet_metric_query_pool_handle_t* phMetricQueryPool
    );

ze_result_t __zecall
zetMetricQueryPoolDestroy(
    zet_metric_query_pool_handle_t hMetricQueryPool
    );

ze_result_t __zecall
zetMetricQueryCreate(
    zet_metric_query_pool_handle_t hMetricQueryPool,
    uint32_t index,
    zet_metric_query_handle_t* phMetricQuery
    );

ze_result_t __zecall
zetMetricQueryDestroy(
    zet_metric_query_handle_t hMetricQuery
    );

ze_result_t __zecall
zetMetricQueryReset(
    zet_metric_query_handle_t hMetricQuery
    );

ze_result_t __zecall
zetCommandListAppendMetricQueryBegin(
    zet_command_list_handle_t hCommandList,
    zet_metric_query_handle_t hMetricQuery
    );

ze_result_t __zecall
zetCommandListAppendMetricQueryEnd(
    zet_command_list_handle_t hCommandList,
    zet_metric_query_handle_t hMetricQuery,
    ze_event_handle_t hCompletionEvent
    );

ze_result_t __zecall
zetCommandListAppendMetricMemoryBarrier(
    zet_command_list_handle_t hCommandList
    );

ze_result_t __zecall
zetMetricQueryGetData(
    zet_metric_query_handle_t hMetricQuery,
    size_t* pRawDataSize,
    uint8_t* pRawData
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_METRIC_H
