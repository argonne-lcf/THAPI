/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_ddi.h
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools
 * @endcond
 *
 */
#ifndef _ZET_DDI_H
#define _ZET_DDI_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zet_api.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef ze_result_t (__zecall *zet_pfnInit_t)(
    ze_init_flag_t
    );

typedef struct _zet_global_dditable_t
{
    zet_pfnInit_t                                               pfnInit;
} zet_global_dditable_t;

__zedllexport ze_result_t __zecall
zetGetGlobalProcAddrTable(
    ze_api_version_t version,
    zet_global_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetGlobalProcAddrTable_t)(
    ze_api_version_t,
    zet_global_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnDeviceActivateMetricGroups_t)(
    zet_device_handle_t,
    uint32_t,
    zet_metric_group_handle_t*
    );

typedef struct _zet_device_dditable_t
{
    zet_pfnDeviceActivateMetricGroups_t                         pfnActivateMetricGroups;
} zet_device_dditable_t;

__zedllexport ze_result_t __zecall
zetGetDeviceProcAddrTable(
    ze_api_version_t version,
    zet_device_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetDeviceProcAddrTable_t)(
    ze_api_version_t,
    zet_device_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnCommandListAppendMetricTracerMarker_t)(
    zet_command_list_handle_t,
    zet_metric_tracer_handle_t,
    uint32_t
    );

typedef ze_result_t (__zecall *zet_pfnCommandListAppendMetricQueryBegin_t)(
    zet_command_list_handle_t,
    zet_metric_query_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnCommandListAppendMetricQueryEnd_t)(
    zet_command_list_handle_t,
    zet_metric_query_handle_t,
    ze_event_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnCommandListAppendMetricMemoryBarrier_t)(
    zet_command_list_handle_t
    );

typedef struct _zet_command_list_dditable_t
{
    zet_pfnCommandListAppendMetricTracerMarker_t                pfnAppendMetricTracerMarker;
    zet_pfnCommandListAppendMetricQueryBegin_t                  pfnAppendMetricQueryBegin;
    zet_pfnCommandListAppendMetricQueryEnd_t                    pfnAppendMetricQueryEnd;
    zet_pfnCommandListAppendMetricMemoryBarrier_t               pfnAppendMetricMemoryBarrier;
} zet_command_list_dditable_t;

__zedllexport ze_result_t __zecall
zetGetCommandListProcAddrTable(
    ze_api_version_t version,
    zet_command_list_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetCommandListProcAddrTable_t)(
    ze_api_version_t,
    zet_command_list_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnModuleGetDebugInfo_t)(
    zet_module_handle_t,
    zet_module_debug_info_format_t,
    size_t*,
    uint8_t*
    );

typedef ze_result_t (__zecall *zet_pfnModuleGetKernelNames_t)(
    zet_module_handle_t,
    uint32_t*,
    const char**
    );

typedef struct _zet_module_dditable_t
{
    zet_pfnModuleGetDebugInfo_t                                 pfnGetDebugInfo;
    zet_pfnModuleGetKernelNames_t                               pfnGetKernelNames;
} zet_module_dditable_t;

__zedllexport ze_result_t __zecall
zetGetModuleProcAddrTable(
    ze_api_version_t version,
    zet_module_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetModuleProcAddrTable_t)(
    ze_api_version_t,
    zet_module_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnKernelGetProfileInfo_t)(
    zet_kernel_handle_t,
    zet_profile_info_t*
    );

typedef struct _zet_kernel_dditable_t
{
    zet_pfnKernelGetProfileInfo_t                               pfnGetProfileInfo;
} zet_kernel_dditable_t;

__zedllexport ze_result_t __zecall
zetGetKernelProcAddrTable(
    ze_api_version_t version,
    zet_kernel_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetKernelProcAddrTable_t)(
    ze_api_version_t,
    zet_kernel_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricGroupGet_t)(
    zet_device_handle_t,
    uint32_t*,
    zet_metric_group_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricGroupGetProperties_t)(
    zet_metric_group_handle_t,
    zet_metric_group_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricGroupCalculateMetricValues_t)(
    zet_metric_group_handle_t,
    size_t,
    const uint8_t*,
    uint32_t*,
    zet_typed_value_t*
    );

typedef struct _zet_metric_group_dditable_t
{
    zet_pfnMetricGroupGet_t                                     pfnGet;
    zet_pfnMetricGroupGetProperties_t                           pfnGetProperties;
    zet_pfnMetricGroupCalculateMetricValues_t                   pfnCalculateMetricValues;
} zet_metric_group_dditable_t;

__zedllexport ze_result_t __zecall
zetGetMetricGroupProcAddrTable(
    ze_api_version_t version,
    zet_metric_group_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetMetricGroupProcAddrTable_t)(
    ze_api_version_t,
    zet_metric_group_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricGet_t)(
    zet_metric_group_handle_t,
    uint32_t*,
    zet_metric_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricGetProperties_t)(
    zet_metric_handle_t,
    zet_metric_properties_t*
    );

typedef struct _zet_metric_dditable_t
{
    zet_pfnMetricGet_t                                          pfnGet;
    zet_pfnMetricGetProperties_t                                pfnGetProperties;
} zet_metric_dditable_t;

__zedllexport ze_result_t __zecall
zetGetMetricProcAddrTable(
    ze_api_version_t version,
    zet_metric_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetMetricProcAddrTable_t)(
    ze_api_version_t,
    zet_metric_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricTracerOpen_t)(
    zet_device_handle_t,
    zet_metric_group_handle_t,
    zet_metric_tracer_desc_t*,
    ze_event_handle_t,
    zet_metric_tracer_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricTracerClose_t)(
    zet_metric_tracer_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnMetricTracerReadData_t)(
    zet_metric_tracer_handle_t,
    uint32_t,
    size_t*,
    uint8_t*
    );

typedef struct _zet_metric_tracer_dditable_t
{
    zet_pfnMetricTracerOpen_t                                   pfnOpen;
    zet_pfnMetricTracerClose_t                                  pfnClose;
    zet_pfnMetricTracerReadData_t                               pfnReadData;
} zet_metric_tracer_dditable_t;

__zedllexport ze_result_t __zecall
zetGetMetricTracerProcAddrTable(
    ze_api_version_t version,
    zet_metric_tracer_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetMetricTracerProcAddrTable_t)(
    ze_api_version_t,
    zet_metric_tracer_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryPoolCreate_t)(
    zet_device_handle_t,
    zet_metric_group_handle_t,
    const zet_metric_query_pool_desc_t*,
    zet_metric_query_pool_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryPoolDestroy_t)(
    zet_metric_query_pool_handle_t
    );

typedef struct _zet_metric_query_pool_dditable_t
{
    zet_pfnMetricQueryPoolCreate_t                              pfnCreate;
    zet_pfnMetricQueryPoolDestroy_t                             pfnDestroy;
} zet_metric_query_pool_dditable_t;

__zedllexport ze_result_t __zecall
zetGetMetricQueryPoolProcAddrTable(
    ze_api_version_t version,
    zet_metric_query_pool_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetMetricQueryPoolProcAddrTable_t)(
    ze_api_version_t,
    zet_metric_query_pool_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryCreate_t)(
    zet_metric_query_pool_handle_t,
    uint32_t,
    zet_metric_query_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryDestroy_t)(
    zet_metric_query_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryReset_t)(
    zet_metric_query_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnMetricQueryGetData_t)(
    zet_metric_query_handle_t,
    size_t*,
    uint8_t*
    );

typedef struct _zet_metric_query_dditable_t
{
    zet_pfnMetricQueryCreate_t                                  pfnCreate;
    zet_pfnMetricQueryDestroy_t                                 pfnDestroy;
    zet_pfnMetricQueryReset_t                                   pfnReset;
    zet_pfnMetricQueryGetData_t                                 pfnGetData;
} zet_metric_query_dditable_t;

__zedllexport ze_result_t __zecall
zetGetMetricQueryProcAddrTable(
    ze_api_version_t version,
    zet_metric_query_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetMetricQueryProcAddrTable_t)(
    ze_api_version_t,
    zet_metric_query_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnTracerCreate_t)(
    zet_device_handle_t,
    const zet_tracer_desc_t*,
    zet_tracer_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnTracerDestroy_t)(
    zet_tracer_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnTracerSetPrologues_t)(
    zet_tracer_handle_t,
    zet_core_callbacks_t*,
    zet_experimental_callbacks_t*
    );

typedef ze_result_t (__zecall *zet_pfnTracerSetEpilogues_t)(
    zet_tracer_handle_t,
    zet_core_callbacks_t*,
    zet_experimental_callbacks_t*
    );

typedef ze_result_t (__zecall *zet_pfnTracerSetEnabled_t)(
    zet_tracer_handle_t,
    ze_bool_t
    );

typedef struct _zet_tracer_dditable_t
{
    zet_pfnTracerCreate_t                                       pfnCreate;
    zet_pfnTracerDestroy_t                                      pfnDestroy;
    zet_pfnTracerSetPrologues_t                                 pfnSetPrologues;
    zet_pfnTracerSetEpilogues_t                                 pfnSetEpilogues;
    zet_pfnTracerSetEnabled_t                                   pfnSetEnabled;
} zet_tracer_dditable_t;

__zedllexport ze_result_t __zecall
zetGetTracerProcAddrTable(
    ze_api_version_t version,
    zet_tracer_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetTracerProcAddrTable_t)(
    ze_api_version_t,
    zet_tracer_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanGet_t)(
    zet_device_handle_t,
    zet_sysman_version_t,
    zet_sysman_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDeviceGetProperties_t)(
    zet_sysman_handle_t,
    zet_sysman_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerGetCurrentMode_t)(
    zet_sysman_handle_t,
    zet_sched_mode_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerGetTimeoutModeProperties_t)(
    zet_sysman_handle_t,
    ze_bool_t,
    zet_sched_timeout_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerGetTimesliceModeProperties_t)(
    zet_sysman_handle_t,
    ze_bool_t,
    zet_sched_timeslice_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerSetTimeoutMode_t)(
    zet_sysman_handle_t,
    zet_sched_timeout_properties_t*,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerSetTimesliceMode_t)(
    zet_sysman_handle_t,
    zet_sched_timeslice_properties_t*,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerSetExclusiveMode_t)(
    zet_sysman_handle_t,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanSchedulerSetComputeUnitDebugMode_t)(
    zet_sysman_handle_t,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanProcessesGetState_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_process_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDeviceReset_t)(
    zet_sysman_handle_t
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDeviceWasRepaired_t)(
    zet_sysman_handle_t,
    ze_bool_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPciGetProperties_t)(
    zet_sysman_handle_t,
    zet_pci_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPciGetState_t)(
    zet_sysman_handle_t,
    zet_pci_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPciGetBarProperties_t)(
    zet_sysman_handle_t,
    uint32_t,
    zet_pci_bar_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPciGetStats_t)(
    zet_sysman_handle_t,
    zet_pci_stats_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_pwr_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_freq_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetAvailableClocks_t)(
    zet_sysman_handle_t,
    uint32_t*,
    double*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEngineGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_engine_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanStandbyGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_standby_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFirmwareGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_firmware_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanMemoryGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_mem_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_fabric_port_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanTemperatureRead_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_temp_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPsuGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_psu_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFanGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_fan_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanLedGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_led_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanRasGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_ras_handle_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEventsGetProperties_t)(
    zet_sysman_handle_t,
    zet_event_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEventsRegister_t)(
    zet_sysman_handle_t,
    uint32_t,
    zet_event_request_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEventsUnregister_t)(
    zet_sysman_handle_t,
    uint32_t,
    zet_event_request_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEventsListen_t)(
    zet_sysman_handle_t,
    ze_bool_t,
    uint32_t,
    uint32_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDiagnosticsGet_t)(
    zet_sysman_handle_t,
    uint32_t*,
    zet_sysman_diag_handle_t*
    );

typedef struct _zet_sysman_dditable_t
{
    zet_pfnSysmanGet_t                                          pfnGet;
    zet_pfnSysmanDeviceGetProperties_t                          pfnDeviceGetProperties;
    zet_pfnSysmanSchedulerGetCurrentMode_t                      pfnSchedulerGetCurrentMode;
    zet_pfnSysmanSchedulerGetTimeoutModeProperties_t            pfnSchedulerGetTimeoutModeProperties;
    zet_pfnSysmanSchedulerGetTimesliceModeProperties_t          pfnSchedulerGetTimesliceModeProperties;
    zet_pfnSysmanSchedulerSetTimeoutMode_t                      pfnSchedulerSetTimeoutMode;
    zet_pfnSysmanSchedulerSetTimesliceMode_t                    pfnSchedulerSetTimesliceMode;
    zet_pfnSysmanSchedulerSetExclusiveMode_t                    pfnSchedulerSetExclusiveMode;
    zet_pfnSysmanSchedulerSetComputeUnitDebugMode_t             pfnSchedulerSetComputeUnitDebugMode;
    zet_pfnSysmanProcessesGetState_t                            pfnProcessesGetState;
    zet_pfnSysmanDeviceReset_t                                  pfnDeviceReset;
    zet_pfnSysmanDeviceWasRepaired_t                            pfnDeviceWasRepaired;
    zet_pfnSysmanPciGetProperties_t                             pfnPciGetProperties;
    zet_pfnSysmanPciGetState_t                                  pfnPciGetState;
    zet_pfnSysmanPciGetBarProperties_t                          pfnPciGetBarProperties;
    zet_pfnSysmanPciGetStats_t                                  pfnPciGetStats;
    zet_pfnSysmanPowerGet_t                                     pfnPowerGet;
    zet_pfnSysmanFrequencyGet_t                                 pfnFrequencyGet;
    zet_pfnSysmanFrequencyGetAvailableClocks_t                  pfnFrequencyGetAvailableClocks;
    zet_pfnSysmanEngineGet_t                                    pfnEngineGet;
    zet_pfnSysmanStandbyGet_t                                   pfnStandbyGet;
    zet_pfnSysmanFirmwareGet_t                                  pfnFirmwareGet;
    zet_pfnSysmanMemoryGet_t                                    pfnMemoryGet;
    zet_pfnSysmanFabricPortGet_t                                pfnFabricPortGet;
    zet_pfnSysmanTemperatureRead_t                              pfnTemperatureRead;
    zet_pfnSysmanPsuGet_t                                       pfnPsuGet;
    zet_pfnSysmanFanGet_t                                       pfnFanGet;
    zet_pfnSysmanLedGet_t                                       pfnLedGet;
    zet_pfnSysmanRasGet_t                                       pfnRasGet;
    zet_pfnSysmanEventsGetProperties_t                          pfnEventsGetProperties;
    zet_pfnSysmanEventsRegister_t                               pfnEventsRegister;
    zet_pfnSysmanEventsUnregister_t                             pfnEventsUnregister;
    zet_pfnSysmanEventsListen_t                                 pfnEventsListen;
    zet_pfnSysmanDiagnosticsGet_t                               pfnDiagnosticsGet;
} zet_sysman_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanProcAddrTable(
    ze_api_version_t version,
    zet_sysman_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerGetProperties_t)(
    zet_sysman_pwr_handle_t,
    zet_power_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerGetEnergyCounter_t)(
    zet_sysman_pwr_handle_t,
    zet_power_energy_counter_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerGetLimits_t)(
    zet_sysman_pwr_handle_t,
    zet_power_sustained_limit_t*,
    zet_power_burst_limit_t*,
    zet_power_peak_limit_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerSetLimits_t)(
    zet_sysman_pwr_handle_t,
    const zet_power_sustained_limit_t*,
    const zet_power_burst_limit_t*,
    const zet_power_peak_limit_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerGetEnergyThreshold_t)(
    zet_sysman_pwr_handle_t,
    zet_energy_threshold_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPowerSetEnergyThreshold_t)(
    zet_sysman_pwr_handle_t,
    double
    );

typedef struct _zet_sysman_power_dditable_t
{
    zet_pfnSysmanPowerGetProperties_t                           pfnGetProperties;
    zet_pfnSysmanPowerGetEnergyCounter_t                        pfnGetEnergyCounter;
    zet_pfnSysmanPowerGetLimits_t                               pfnGetLimits;
    zet_pfnSysmanPowerSetLimits_t                               pfnSetLimits;
    zet_pfnSysmanPowerGetEnergyThreshold_t                      pfnGetEnergyThreshold;
    zet_pfnSysmanPowerSetEnergyThreshold_t                      pfnSetEnergyThreshold;
} zet_sysman_power_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanPowerProcAddrTable(
    ze_api_version_t version,
    zet_sysman_power_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanPowerProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_power_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetProperties_t)(
    zet_sysman_freq_handle_t,
    zet_freq_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetRange_t)(
    zet_sysman_freq_handle_t,
    zet_freq_range_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencySetRange_t)(
    zet_sysman_freq_handle_t,
    const zet_freq_range_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetState_t)(
    zet_sysman_freq_handle_t,
    zet_freq_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetThrottleTime_t)(
    zet_sysman_freq_handle_t,
    zet_freq_throttle_time_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetOcCapabilities_t)(
    zet_sysman_freq_handle_t,
    zet_oc_capabilities_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetOcConfig_t)(
    zet_sysman_freq_handle_t,
    zet_oc_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencySetOcConfig_t)(
    zet_sysman_freq_handle_t,
    zet_oc_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetOcIccMax_t)(
    zet_sysman_freq_handle_t,
    double*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencySetOcIccMax_t)(
    zet_sysman_freq_handle_t,
    double
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencyGetOcTjMax_t)(
    zet_sysman_freq_handle_t,
    double*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFrequencySetOcTjMax_t)(
    zet_sysman_freq_handle_t,
    double
    );

typedef struct _zet_sysman_frequency_dditable_t
{
    zet_pfnSysmanFrequencyGetProperties_t                       pfnGetProperties;
    zet_pfnSysmanFrequencyGetRange_t                            pfnGetRange;
    zet_pfnSysmanFrequencySetRange_t                            pfnSetRange;
    zet_pfnSysmanFrequencyGetState_t                            pfnGetState;
    zet_pfnSysmanFrequencyGetThrottleTime_t                     pfnGetThrottleTime;
    zet_pfnSysmanFrequencyGetOcCapabilities_t                   pfnGetOcCapabilities;
    zet_pfnSysmanFrequencyGetOcConfig_t                         pfnGetOcConfig;
    zet_pfnSysmanFrequencySetOcConfig_t                         pfnSetOcConfig;
    zet_pfnSysmanFrequencyGetOcIccMax_t                         pfnGetOcIccMax;
    zet_pfnSysmanFrequencySetOcIccMax_t                         pfnSetOcIccMax;
    zet_pfnSysmanFrequencyGetOcTjMax_t                          pfnGetOcTjMax;
    zet_pfnSysmanFrequencySetOcTjMax_t                          pfnSetOcTjMax;
} zet_sysman_frequency_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanFrequencyProcAddrTable(
    ze_api_version_t version,
    zet_sysman_frequency_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanFrequencyProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_frequency_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEngineGetProperties_t)(
    zet_sysman_engine_handle_t,
    zet_engine_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanEngineGetActivity_t)(
    zet_sysman_engine_handle_t,
    zet_engine_stats_t*
    );

typedef struct _zet_sysman_engine_dditable_t
{
    zet_pfnSysmanEngineGetProperties_t                          pfnGetProperties;
    zet_pfnSysmanEngineGetActivity_t                            pfnGetActivity;
} zet_sysman_engine_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanEngineProcAddrTable(
    ze_api_version_t version,
    zet_sysman_engine_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanEngineProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_engine_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanStandbyGetProperties_t)(
    zet_sysman_standby_handle_t,
    zet_standby_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanStandbyGetMode_t)(
    zet_sysman_standby_handle_t,
    zet_standby_promo_mode_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanStandbySetMode_t)(
    zet_sysman_standby_handle_t,
    zet_standby_promo_mode_t
    );

typedef struct _zet_sysman_standby_dditable_t
{
    zet_pfnSysmanStandbyGetProperties_t                         pfnGetProperties;
    zet_pfnSysmanStandbyGetMode_t                               pfnGetMode;
    zet_pfnSysmanStandbySetMode_t                               pfnSetMode;
} zet_sysman_standby_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanStandbyProcAddrTable(
    ze_api_version_t version,
    zet_sysman_standby_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanStandbyProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_standby_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFirmwareGetProperties_t)(
    zet_sysman_firmware_handle_t,
    zet_firmware_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFirmwareGetChecksum_t)(
    zet_sysman_firmware_handle_t,
    uint32_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFirmwareFlash_t)(
    zet_sysman_firmware_handle_t,
    void*,
    uint32_t
    );

typedef struct _zet_sysman_firmware_dditable_t
{
    zet_pfnSysmanFirmwareGetProperties_t                        pfnGetProperties;
    zet_pfnSysmanFirmwareGetChecksum_t                          pfnGetChecksum;
    zet_pfnSysmanFirmwareFlash_t                                pfnFlash;
} zet_sysman_firmware_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanFirmwareProcAddrTable(
    ze_api_version_t version,
    zet_sysman_firmware_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanFirmwareProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_firmware_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanMemoryGetProperties_t)(
    zet_sysman_mem_handle_t,
    zet_mem_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanMemoryGetState_t)(
    zet_sysman_mem_handle_t,
    zet_mem_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanMemoryGetBandwidth_t)(
    zet_sysman_mem_handle_t,
    zet_mem_bandwidth_t*
    );

typedef struct _zet_sysman_memory_dditable_t
{
    zet_pfnSysmanMemoryGetProperties_t                          pfnGetProperties;
    zet_pfnSysmanMemoryGetState_t                               pfnGetState;
    zet_pfnSysmanMemoryGetBandwidth_t                           pfnGetBandwidth;
} zet_sysman_memory_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanMemoryProcAddrTable(
    ze_api_version_t version,
    zet_sysman_memory_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanMemoryProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_memory_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGetProperties_t)(
    zet_sysman_fabric_port_handle_t,
    zet_fabric_port_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGetLinkType_t)(
    zet_sysman_fabric_port_handle_t,
    ze_bool_t,
    zet_fabric_link_type_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGetConfig_t)(
    zet_sysman_fabric_port_handle_t,
    zet_fabric_port_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortSetConfig_t)(
    zet_sysman_fabric_port_handle_t,
    zet_fabric_port_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGetState_t)(
    zet_sysman_fabric_port_handle_t,
    zet_fabric_port_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFabricPortGetThroughput_t)(
    zet_sysman_fabric_port_handle_t,
    zet_fabric_port_throughput_t*
    );

typedef struct _zet_sysman_fabric_port_dditable_t
{
    zet_pfnSysmanFabricPortGetProperties_t                      pfnGetProperties;
    zet_pfnSysmanFabricPortGetLinkType_t                        pfnGetLinkType;
    zet_pfnSysmanFabricPortGetConfig_t                          pfnGetConfig;
    zet_pfnSysmanFabricPortSetConfig_t                          pfnSetConfig;
    zet_pfnSysmanFabricPortGetState_t                           pfnGetState;
    zet_pfnSysmanFabricPortGetThroughput_t                      pfnGetThroughput;
} zet_sysman_fabric_port_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanFabricPortProcAddrTable(
    ze_api_version_t version,
    zet_sysman_fabric_port_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanFabricPortProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_fabric_port_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanTemperatureGetProperties_t)(
    zet_sysman_temp_handle_t,
    zet_temp_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanTemperatureGet_t)(
    zet_sysman_temp_handle_t,
    double*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanTemperatureGetThresholds_t)(
    zet_sysman_temp_handle_t,
    zet_temp_threshold_t*,
    zet_temp_threshold_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanTemperatureSetThresholds_t)(
    zet_sysman_temp_handle_t,
    double,
    double
    );

typedef struct _zet_sysman_temperature_dditable_t
{
    zet_pfnSysmanTemperatureGetProperties_t                     pfnGetProperties;
    zet_pfnSysmanTemperatureGet_t                               pfnGet;
    zet_pfnSysmanTemperatureGetThresholds_t                     pfnGetThresholds;
    zet_pfnSysmanTemperatureSetThresholds_t                     pfnSetThresholds;
} zet_sysman_temperature_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanTemperatureProcAddrTable(
    ze_api_version_t version,
    zet_sysman_temperature_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanTemperatureProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_temperature_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPsuGetProperties_t)(
    zet_sysman_psu_handle_t,
    zet_psu_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanPsuGetState_t)(
    zet_sysman_psu_handle_t,
    zet_psu_state_t*
    );

typedef struct _zet_sysman_psu_dditable_t
{
    zet_pfnSysmanPsuGetProperties_t                             pfnGetProperties;
    zet_pfnSysmanPsuGetState_t                                  pfnGetState;
} zet_sysman_psu_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanPsuProcAddrTable(
    ze_api_version_t version,
    zet_sysman_psu_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanPsuProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_psu_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFanGetProperties_t)(
    zet_sysman_fan_handle_t,
    zet_fan_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFanGetConfig_t)(
    zet_sysman_fan_handle_t,
    zet_fan_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFanSetConfig_t)(
    zet_sysman_fan_handle_t,
    const zet_fan_config_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanFanGetState_t)(
    zet_sysman_fan_handle_t,
    zet_fan_speed_units_t,
    zet_fan_state_t*
    );

typedef struct _zet_sysman_fan_dditable_t
{
    zet_pfnSysmanFanGetProperties_t                             pfnGetProperties;
    zet_pfnSysmanFanGetConfig_t                                 pfnGetConfig;
    zet_pfnSysmanFanSetConfig_t                                 pfnSetConfig;
    zet_pfnSysmanFanGetState_t                                  pfnGetState;
} zet_sysman_fan_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanFanProcAddrTable(
    ze_api_version_t version,
    zet_sysman_fan_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanFanProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_fan_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanLedGetProperties_t)(
    zet_sysman_led_handle_t,
    zet_led_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanLedGetState_t)(
    zet_sysman_led_handle_t,
    zet_led_state_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanLedSetState_t)(
    zet_sysman_led_handle_t,
    const zet_led_state_t*
    );

typedef struct _zet_sysman_led_dditable_t
{
    zet_pfnSysmanLedGetProperties_t                             pfnGetProperties;
    zet_pfnSysmanLedGetState_t                                  pfnGetState;
    zet_pfnSysmanLedSetState_t                                  pfnSetState;
} zet_sysman_led_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanLedProcAddrTable(
    ze_api_version_t version,
    zet_sysman_led_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanLedProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_led_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanRasGetProperties_t)(
    zet_sysman_ras_handle_t,
    zet_ras_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanRasGetErrors_t)(
    zet_sysman_ras_handle_t,
    ze_bool_t,
    uint64_t*,
    zet_ras_details_t*
    );

typedef struct _zet_sysman_ras_dditable_t
{
    zet_pfnSysmanRasGetProperties_t                             pfnGetProperties;
    zet_pfnSysmanRasGetErrors_t                                 pfnGetErrors;
} zet_sysman_ras_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanRasProcAddrTable(
    ze_api_version_t version,
    zet_sysman_ras_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanRasProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_ras_dditable_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDiagnosticsGetProperties_t)(
    zet_sysman_diag_handle_t,
    zet_diag_properties_t*
    );

typedef ze_result_t (__zecall *zet_pfnSysmanDiagnosticsRunTests_t)(
    zet_sysman_diag_handle_t,
    uint32_t,
    uint32_t,
    zet_diag_result_t*
    );

typedef struct _zet_sysman_diagnostics_dditable_t
{
    zet_pfnSysmanDiagnosticsGetProperties_t                     pfnGetProperties;
    zet_pfnSysmanDiagnosticsRunTests_t                          pfnRunTests;
} zet_sysman_diagnostics_dditable_t;

__zedllexport ze_result_t __zecall
zetGetSysmanDiagnosticsProcAddrTable(
    ze_api_version_t version,
    zet_sysman_diagnostics_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zet_pfnGetSysmanDiagnosticsProcAddrTable_t)(
    ze_api_version_t,
    zet_sysman_diagnostics_dditable_t*
    );

typedef struct _zet_dditable_t
{
    zet_global_dditable_t               Global;
    zet_device_dditable_t               Device;
    zet_command_list_dditable_t         CommandList;
    zet_module_dditable_t               Module;
    zet_kernel_dditable_t               Kernel;
    zet_metric_group_dditable_t         MetricGroup;
    zet_metric_dditable_t               Metric;
    zet_metric_tracer_dditable_t        MetricTracer;
    zet_metric_query_pool_dditable_t    MetricQueryPool;
    zet_metric_query_dditable_t         MetricQuery;
    zet_tracer_dditable_t               Tracer;
    zet_sysman_dditable_t               Sysman;
    zet_sysman_power_dditable_t         SysmanPower;
    zet_sysman_frequency_dditable_t     SysmanFrequency;
    zet_sysman_engine_dditable_t        SysmanEngine;
    zet_sysman_standby_dditable_t       SysmanStandby;
    zet_sysman_firmware_dditable_t      SysmanFirmware;
    zet_sysman_memory_dditable_t        SysmanMemory;
    zet_sysman_fabric_port_dditable_t   SysmanFabricPort;
    zet_sysman_temperature_dditable_t   SysmanTemperature;
    zet_sysman_psu_dditable_t           SysmanPsu;
    zet_sysman_fan_dditable_t           SysmanFan;
    zet_sysman_led_dditable_t           SysmanLed;
    zet_sysman_ras_dditable_t           SysmanRas;
    zet_sysman_diagnostics_dditable_t   SysmanDiagnostics;
} zet_dditable_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_DDI_H
