/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zes_ddi.h
 * @version v1.9-r1.9.3
 *
 */
#ifndef _ZES_DDI_VER_H
#define _ZES_DDI_VER_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zes_ddi.h"

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// [1.5]
/// @brief Table of Global functions pointers
typedef struct _zes_global_dditable_t_1_5
{
    zes_pfnInit_t                                               pfnInit;
} zes_global_dditable_t_1_5;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.4, 1.5, 1.7]
/// @brief Table of Device functions pointers
typedef struct _zes_device_dditable_t_1_0
{
    zes_pfnDeviceGetProperties_t                                pfnGetProperties;
    zes_pfnDeviceGetState_t                                     pfnGetState;
    zes_pfnDeviceReset_t                                        pfnReset;
    zes_pfnDeviceProcessesGetState_t                            pfnProcessesGetState;
    zes_pfnDevicePciGetProperties_t                             pfnPciGetProperties;
    zes_pfnDevicePciGetState_t                                  pfnPciGetState;
    zes_pfnDevicePciGetBars_t                                   pfnPciGetBars;
    zes_pfnDevicePciGetStats_t                                  pfnPciGetStats;
    zes_pfnDeviceEnumDiagnosticTestSuites_t                     pfnEnumDiagnosticTestSuites;
    zes_pfnDeviceEnumEngineGroups_t                             pfnEnumEngineGroups;
    zes_pfnDeviceEventRegister_t                                pfnEventRegister;
    zes_pfnDeviceEnumFabricPorts_t                              pfnEnumFabricPorts;
    zes_pfnDeviceEnumFans_t                                     pfnEnumFans;
    zes_pfnDeviceEnumFirmwares_t                                pfnEnumFirmwares;
    zes_pfnDeviceEnumFrequencyDomains_t                         pfnEnumFrequencyDomains;
    zes_pfnDeviceEnumLeds_t                                     pfnEnumLeds;
    zes_pfnDeviceEnumMemoryModules_t                            pfnEnumMemoryModules;
    zes_pfnDeviceEnumPerformanceFactorDomains_t                 pfnEnumPerformanceFactorDomains;
    zes_pfnDeviceEnumPowerDomains_t                             pfnEnumPowerDomains;
    zes_pfnDeviceGetCardPowerDomain_t                           pfnGetCardPowerDomain;
    zes_pfnDeviceEnumPsus_t                                     pfnEnumPsus;
    zes_pfnDeviceEnumRasErrorSets_t                             pfnEnumRasErrorSets;
    zes_pfnDeviceEnumSchedulers_t                               pfnEnumSchedulers;
    zes_pfnDeviceEnumStandbyDomains_t                           pfnEnumStandbyDomains;
    zes_pfnDeviceEnumTemperatureSensors_t                       pfnEnumTemperatureSensors;
} zes_device_dditable_t_1_0;

typedef struct _zes_device_dditable_t_1_4
{
    zes_pfnDeviceGetProperties_t                                pfnGetProperties;
    zes_pfnDeviceGetState_t                                     pfnGetState;
    zes_pfnDeviceReset_t                                        pfnReset;
    zes_pfnDeviceProcessesGetState_t                            pfnProcessesGetState;
    zes_pfnDevicePciGetProperties_t                             pfnPciGetProperties;
    zes_pfnDevicePciGetState_t                                  pfnPciGetState;
    zes_pfnDevicePciGetBars_t                                   pfnPciGetBars;
    zes_pfnDevicePciGetStats_t                                  pfnPciGetStats;
    zes_pfnDeviceEnumDiagnosticTestSuites_t                     pfnEnumDiagnosticTestSuites;
    zes_pfnDeviceEnumEngineGroups_t                             pfnEnumEngineGroups;
    zes_pfnDeviceEventRegister_t                                pfnEventRegister;
    zes_pfnDeviceEnumFabricPorts_t                              pfnEnumFabricPorts;
    zes_pfnDeviceEnumFans_t                                     pfnEnumFans;
    zes_pfnDeviceEnumFirmwares_t                                pfnEnumFirmwares;
    zes_pfnDeviceEnumFrequencyDomains_t                         pfnEnumFrequencyDomains;
    zes_pfnDeviceEnumLeds_t                                     pfnEnumLeds;
    zes_pfnDeviceEnumMemoryModules_t                            pfnEnumMemoryModules;
    zes_pfnDeviceEnumPerformanceFactorDomains_t                 pfnEnumPerformanceFactorDomains;
    zes_pfnDeviceEnumPowerDomains_t                             pfnEnumPowerDomains;
    zes_pfnDeviceGetCardPowerDomain_t                           pfnGetCardPowerDomain;
    zes_pfnDeviceEnumPsus_t                                     pfnEnumPsus;
    zes_pfnDeviceEnumRasErrorSets_t                             pfnEnumRasErrorSets;
    zes_pfnDeviceEnumSchedulers_t                               pfnEnumSchedulers;
    zes_pfnDeviceEnumStandbyDomains_t                           pfnEnumStandbyDomains;
    zes_pfnDeviceEnumTemperatureSensors_t                       pfnEnumTemperatureSensors;
    zes_pfnDeviceEccAvailable_t                                 pfnEccAvailable;
    zes_pfnDeviceEccConfigurable_t                              pfnEccConfigurable;
    zes_pfnDeviceGetEccState_t                                  pfnGetEccState;
    zes_pfnDeviceSetEccState_t                                  pfnSetEccState;
} zes_device_dditable_t_1_4;

typedef struct _zes_device_dditable_t_1_5
{
    zes_pfnDeviceGetProperties_t                                pfnGetProperties;
    zes_pfnDeviceGetState_t                                     pfnGetState;
    zes_pfnDeviceReset_t                                        pfnReset;
    zes_pfnDeviceProcessesGetState_t                            pfnProcessesGetState;
    zes_pfnDevicePciGetProperties_t                             pfnPciGetProperties;
    zes_pfnDevicePciGetState_t                                  pfnPciGetState;
    zes_pfnDevicePciGetBars_t                                   pfnPciGetBars;
    zes_pfnDevicePciGetStats_t                                  pfnPciGetStats;
    zes_pfnDeviceEnumDiagnosticTestSuites_t                     pfnEnumDiagnosticTestSuites;
    zes_pfnDeviceEnumEngineGroups_t                             pfnEnumEngineGroups;
    zes_pfnDeviceEventRegister_t                                pfnEventRegister;
    zes_pfnDeviceEnumFabricPorts_t                              pfnEnumFabricPorts;
    zes_pfnDeviceEnumFans_t                                     pfnEnumFans;
    zes_pfnDeviceEnumFirmwares_t                                pfnEnumFirmwares;
    zes_pfnDeviceEnumFrequencyDomains_t                         pfnEnumFrequencyDomains;
    zes_pfnDeviceEnumLeds_t                                     pfnEnumLeds;
    zes_pfnDeviceEnumMemoryModules_t                            pfnEnumMemoryModules;
    zes_pfnDeviceEnumPerformanceFactorDomains_t                 pfnEnumPerformanceFactorDomains;
    zes_pfnDeviceEnumPowerDomains_t                             pfnEnumPowerDomains;
    zes_pfnDeviceGetCardPowerDomain_t                           pfnGetCardPowerDomain;
    zes_pfnDeviceEnumPsus_t                                     pfnEnumPsus;
    zes_pfnDeviceEnumRasErrorSets_t                             pfnEnumRasErrorSets;
    zes_pfnDeviceEnumSchedulers_t                               pfnEnumSchedulers;
    zes_pfnDeviceEnumStandbyDomains_t                           pfnEnumStandbyDomains;
    zes_pfnDeviceEnumTemperatureSensors_t                       pfnEnumTemperatureSensors;
    zes_pfnDeviceEccAvailable_t                                 pfnEccAvailable;
    zes_pfnDeviceEccConfigurable_t                              pfnEccConfigurable;
    zes_pfnDeviceGetEccState_t                                  pfnGetEccState;
    zes_pfnDeviceSetEccState_t                                  pfnSetEccState;
    zes_pfnDeviceGet_t                                          pfnGet;
    zes_pfnDeviceSetOverclockWaiver_t                           pfnSetOverclockWaiver;
    zes_pfnDeviceGetOverclockDomains_t                          pfnGetOverclockDomains;
    zes_pfnDeviceGetOverclockControls_t                         pfnGetOverclockControls;
    zes_pfnDeviceResetOverclockSettings_t                       pfnResetOverclockSettings;
    zes_pfnDeviceReadOverclockState_t                           pfnReadOverclockState;
    zes_pfnDeviceEnumOverclockDomains_t                         pfnEnumOverclockDomains;
} zes_device_dditable_t_1_5;

typedef struct _zes_device_dditable_t_1_7
{
    zes_pfnDeviceGetProperties_t                                pfnGetProperties;
    zes_pfnDeviceGetState_t                                     pfnGetState;
    zes_pfnDeviceReset_t                                        pfnReset;
    zes_pfnDeviceProcessesGetState_t                            pfnProcessesGetState;
    zes_pfnDevicePciGetProperties_t                             pfnPciGetProperties;
    zes_pfnDevicePciGetState_t                                  pfnPciGetState;
    zes_pfnDevicePciGetBars_t                                   pfnPciGetBars;
    zes_pfnDevicePciGetStats_t                                  pfnPciGetStats;
    zes_pfnDeviceEnumDiagnosticTestSuites_t                     pfnEnumDiagnosticTestSuites;
    zes_pfnDeviceEnumEngineGroups_t                             pfnEnumEngineGroups;
    zes_pfnDeviceEventRegister_t                                pfnEventRegister;
    zes_pfnDeviceEnumFabricPorts_t                              pfnEnumFabricPorts;
    zes_pfnDeviceEnumFans_t                                     pfnEnumFans;
    zes_pfnDeviceEnumFirmwares_t                                pfnEnumFirmwares;
    zes_pfnDeviceEnumFrequencyDomains_t                         pfnEnumFrequencyDomains;
    zes_pfnDeviceEnumLeds_t                                     pfnEnumLeds;
    zes_pfnDeviceEnumMemoryModules_t                            pfnEnumMemoryModules;
    zes_pfnDeviceEnumPerformanceFactorDomains_t                 pfnEnumPerformanceFactorDomains;
    zes_pfnDeviceEnumPowerDomains_t                             pfnEnumPowerDomains;
    zes_pfnDeviceGetCardPowerDomain_t                           pfnGetCardPowerDomain;
    zes_pfnDeviceEnumPsus_t                                     pfnEnumPsus;
    zes_pfnDeviceEnumRasErrorSets_t                             pfnEnumRasErrorSets;
    zes_pfnDeviceEnumSchedulers_t                               pfnEnumSchedulers;
    zes_pfnDeviceEnumStandbyDomains_t                           pfnEnumStandbyDomains;
    zes_pfnDeviceEnumTemperatureSensors_t                       pfnEnumTemperatureSensors;
    zes_pfnDeviceEccAvailable_t                                 pfnEccAvailable;
    zes_pfnDeviceEccConfigurable_t                              pfnEccConfigurable;
    zes_pfnDeviceGetEccState_t                                  pfnGetEccState;
    zes_pfnDeviceSetEccState_t                                  pfnSetEccState;
    zes_pfnDeviceGet_t                                          pfnGet;
    zes_pfnDeviceSetOverclockWaiver_t                           pfnSetOverclockWaiver;
    zes_pfnDeviceGetOverclockDomains_t                          pfnGetOverclockDomains;
    zes_pfnDeviceGetOverclockControls_t                         pfnGetOverclockControls;
    zes_pfnDeviceResetOverclockSettings_t                       pfnResetOverclockSettings;
    zes_pfnDeviceReadOverclockState_t                           pfnReadOverclockState;
    zes_pfnDeviceEnumOverclockDomains_t                         pfnEnumOverclockDomains;
    zes_pfnDeviceResetExt_t                                     pfnResetExt;
} zes_device_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of DeviceExp functions pointers
typedef struct _zes_device_exp_dditable_t_1_9
{
    zes_pfnDeviceGetSubDevicePropertiesExp_t                    pfnGetSubDevicePropertiesExp;
    zes_pfnDeviceEnumActiveVFExp_t                              pfnEnumActiveVFExp;
} zes_device_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.1, 1.5, 1.8]
/// @brief Table of Driver functions pointers
typedef struct _zes_driver_dditable_t_1_0
{
    zes_pfnDriverEventListen_t                                  pfnEventListen;
} zes_driver_dditable_t_1_0;

typedef struct _zes_driver_dditable_t_1_1
{
    zes_pfnDriverEventListen_t                                  pfnEventListen;
    zes_pfnDriverEventListenEx_t                                pfnEventListenEx;
} zes_driver_dditable_t_1_1;

typedef struct _zes_driver_dditable_t_1_5
{
    zes_pfnDriverEventListen_t                                  pfnEventListen;
    zes_pfnDriverEventListenEx_t                                pfnEventListenEx;
    zes_pfnDriverGet_t                                          pfnGet;
} zes_driver_dditable_t_1_5;

typedef struct _zes_driver_dditable_t_1_8
{
    zes_pfnDriverEventListen_t                                  pfnEventListen;
    zes_pfnDriverEventListenEx_t                                pfnEventListenEx;
    zes_pfnDriverGet_t                                          pfnGet;
    zes_pfnDriverGetExtensionProperties_t                       pfnGetExtensionProperties;
    zes_pfnDriverGetExtensionFunctionAddress_t                  pfnGetExtensionFunctionAddress;
} zes_driver_dditable_t_1_8;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of DriverExp functions pointers
typedef struct _zes_driver_exp_dditable_t_1_9
{
    zes_pfnDriverGetDeviceByUuidExp_t                           pfnGetDeviceByUuidExp;
} zes_driver_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.5]
/// @brief Table of Overclock functions pointers
typedef struct _zes_overclock_dditable_t_1_5
{
    zes_pfnOverclockGetDomainProperties_t                       pfnGetDomainProperties;
    zes_pfnOverclockGetDomainVFProperties_t                     pfnGetDomainVFProperties;
    zes_pfnOverclockGetDomainControlProperties_t                pfnGetDomainControlProperties;
    zes_pfnOverclockGetControlCurrentValue_t                    pfnGetControlCurrentValue;
    zes_pfnOverclockGetControlPendingValue_t                    pfnGetControlPendingValue;
    zes_pfnOverclockSetControlUserValue_t                       pfnSetControlUserValue;
    zes_pfnOverclockGetControlState_t                           pfnGetControlState;
    zes_pfnOverclockGetVFPointValues_t                          pfnGetVFPointValues;
    zes_pfnOverclockSetVFPointValues_t                          pfnSetVFPointValues;
} zes_overclock_dditable_t_1_5;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Scheduler functions pointers
typedef struct _zes_scheduler_dditable_t_1_0
{
    zes_pfnSchedulerGetProperties_t                             pfnGetProperties;
    zes_pfnSchedulerGetCurrentMode_t                            pfnGetCurrentMode;
    zes_pfnSchedulerGetTimeoutModeProperties_t                  pfnGetTimeoutModeProperties;
    zes_pfnSchedulerGetTimesliceModeProperties_t                pfnGetTimesliceModeProperties;
    zes_pfnSchedulerSetTimeoutMode_t                            pfnSetTimeoutMode;
    zes_pfnSchedulerSetTimesliceMode_t                          pfnSetTimesliceMode;
    zes_pfnSchedulerSetExclusiveMode_t                          pfnSetExclusiveMode;
    zes_pfnSchedulerSetComputeUnitDebugMode_t                   pfnSetComputeUnitDebugMode;
} zes_scheduler_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of PerformanceFactor functions pointers
typedef struct _zes_performance_factor_dditable_t_1_0
{
    zes_pfnPerformanceFactorGetProperties_t                     pfnGetProperties;
    zes_pfnPerformanceFactorGetConfig_t                         pfnGetConfig;
    zes_pfnPerformanceFactorSetConfig_t                         pfnSetConfig;
} zes_performance_factor_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Power functions pointers
typedef struct _zes_power_dditable_t_1_0
{
    zes_pfnPowerGetProperties_t                                 pfnGetProperties;
    zes_pfnPowerGetEnergyCounter_t                              pfnGetEnergyCounter;
    zes_pfnPowerGetLimits_t                                     pfnGetLimits;
    zes_pfnPowerSetLimits_t                                     pfnSetLimits;
    zes_pfnPowerGetEnergyThreshold_t                            pfnGetEnergyThreshold;
    zes_pfnPowerSetEnergyThreshold_t                            pfnSetEnergyThreshold;
    zes_pfnPowerGetLimitsExt_t                                  pfnGetLimitsExt;
    zes_pfnPowerSetLimitsExt_t                                  pfnSetLimitsExt;
} zes_power_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Frequency functions pointers
typedef struct _zes_frequency_dditable_t_1_0
{
    zes_pfnFrequencyGetProperties_t                             pfnGetProperties;
    zes_pfnFrequencyGetAvailableClocks_t                        pfnGetAvailableClocks;
    zes_pfnFrequencyGetRange_t                                  pfnGetRange;
    zes_pfnFrequencySetRange_t                                  pfnSetRange;
    zes_pfnFrequencyGetState_t                                  pfnGetState;
    zes_pfnFrequencyGetThrottleTime_t                           pfnGetThrottleTime;
    zes_pfnFrequencyOcGetCapabilities_t                         pfnOcGetCapabilities;
    zes_pfnFrequencyOcGetFrequencyTarget_t                      pfnOcGetFrequencyTarget;
    zes_pfnFrequencyOcSetFrequencyTarget_t                      pfnOcSetFrequencyTarget;
    zes_pfnFrequencyOcGetVoltageTarget_t                        pfnOcGetVoltageTarget;
    zes_pfnFrequencyOcSetVoltageTarget_t                        pfnOcSetVoltageTarget;
    zes_pfnFrequencyOcSetMode_t                                 pfnOcSetMode;
    zes_pfnFrequencyOcGetMode_t                                 pfnOcGetMode;
    zes_pfnFrequencyOcGetIccMax_t                               pfnOcGetIccMax;
    zes_pfnFrequencyOcSetIccMax_t                               pfnOcSetIccMax;
    zes_pfnFrequencyOcGetTjMax_t                                pfnOcGetTjMax;
    zes_pfnFrequencyOcSetTjMax_t                                pfnOcSetTjMax;
} zes_frequency_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.7]
/// @brief Table of Engine functions pointers
typedef struct _zes_engine_dditable_t_1_0
{
    zes_pfnEngineGetProperties_t                                pfnGetProperties;
} zes_engine_dditable_t_1_0;

typedef struct _zes_engine_dditable_t_1_7
{
    zes_pfnEngineGetProperties_t                                pfnGetProperties;
    zes_pfnEngineGetActivity_t                                  pfnGetActivity;
    zes_pfnEngineGetActivityExt_t                               pfnGetActivityExt;
} zes_engine_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Standby functions pointers
typedef struct _zes_standby_dditable_t_1_0
{
    zes_pfnStandbyGetProperties_t                               pfnGetProperties;
    zes_pfnStandbyGetMode_t                                     pfnGetMode;
    zes_pfnStandbySetMode_t                                     pfnSetMode;
} zes_standby_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.8, 1.9]
/// @brief Table of Firmware functions pointers
typedef struct _zes_firmware_dditable_t_1_0
{
    zes_pfnFirmwareGetProperties_t                              pfnGetProperties;
    zes_pfnFirmwareFlash_t                                      pfnFlash;
} zes_firmware_dditable_t_1_0;

typedef struct _zes_firmware_dditable_t_1_8
{
    zes_pfnFirmwareGetProperties_t                              pfnGetProperties;
    zes_pfnFirmwareFlash_t                                      pfnFlash;
    zes_pfnFirmwareGetFlashProgress_t                           pfnGetFlashProgress;
} zes_firmware_dditable_t_1_8;

typedef struct _zes_firmware_dditable_t_1_9
{
    zes_pfnFirmwareGetProperties_t                              pfnGetProperties;
    zes_pfnFirmwareFlash_t                                      pfnFlash;
    zes_pfnFirmwareGetFlashProgress_t                           pfnGetFlashProgress;
    zes_pfnFirmwareGetConsoleLogs_t                             pfnGetConsoleLogs;
} zes_firmware_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of FirmwareExp functions pointers
typedef struct _zes_firmware_exp_dditable_t_1_9
{
    zes_pfnFirmwareGetSecurityVersionExp_t                      pfnGetSecurityVersionExp;
    zes_pfnFirmwareSetSecurityVersionExp_t                      pfnSetSecurityVersionExp;
} zes_firmware_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Memory functions pointers
typedef struct _zes_memory_dditable_t_1_0
{
    zes_pfnMemoryGetProperties_t                                pfnGetProperties;
    zes_pfnMemoryGetState_t                                     pfnGetState;
    zes_pfnMemoryGetBandwidth_t                                 pfnGetBandwidth;
} zes_memory_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.7]
/// @brief Table of FabricPort functions pointers
typedef struct _zes_fabric_port_dditable_t_1_0
{
    zes_pfnFabricPortGetProperties_t                            pfnGetProperties;
    zes_pfnFabricPortGetLinkType_t                              pfnGetLinkType;
    zes_pfnFabricPortGetConfig_t                                pfnGetConfig;
    zes_pfnFabricPortSetConfig_t                                pfnSetConfig;
    zes_pfnFabricPortGetState_t                                 pfnGetState;
    zes_pfnFabricPortGetThroughput_t                            pfnGetThroughput;
} zes_fabric_port_dditable_t_1_0;

typedef struct _zes_fabric_port_dditable_t_1_7
{
    zes_pfnFabricPortGetProperties_t                            pfnGetProperties;
    zes_pfnFabricPortGetLinkType_t                              pfnGetLinkType;
    zes_pfnFabricPortGetConfig_t                                pfnGetConfig;
    zes_pfnFabricPortSetConfig_t                                pfnSetConfig;
    zes_pfnFabricPortGetState_t                                 pfnGetState;
    zes_pfnFabricPortGetThroughput_t                            pfnGetThroughput;
    zes_pfnFabricPortGetFabricErrorCounters_t                   pfnGetFabricErrorCounters;
    zes_pfnFabricPortGetMultiPortThroughput_t                   pfnGetMultiPortThroughput;
} zes_fabric_port_dditable_t_1_7;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Temperature functions pointers
typedef struct _zes_temperature_dditable_t_1_0
{
    zes_pfnTemperatureGetProperties_t                           pfnGetProperties;
    zes_pfnTemperatureGetConfig_t                               pfnGetConfig;
    zes_pfnTemperatureSetConfig_t                               pfnSetConfig;
    zes_pfnTemperatureGetState_t                                pfnGetState;
} zes_temperature_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Psu functions pointers
typedef struct _zes_psu_dditable_t_1_0
{
    zes_pfnPsuGetProperties_t                                   pfnGetProperties;
    zes_pfnPsuGetState_t                                        pfnGetState;
} zes_psu_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Fan functions pointers
typedef struct _zes_fan_dditable_t_1_0
{
    zes_pfnFanGetProperties_t                                   pfnGetProperties;
    zes_pfnFanGetConfig_t                                       pfnGetConfig;
    zes_pfnFanSetDefaultMode_t                                  pfnSetDefaultMode;
    zes_pfnFanSetFixedSpeedMode_t                               pfnSetFixedSpeedMode;
    zes_pfnFanSetSpeedTableMode_t                               pfnSetSpeedTableMode;
    zes_pfnFanGetState_t                                        pfnGetState;
} zes_fan_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Led functions pointers
typedef struct _zes_led_dditable_t_1_0
{
    zes_pfnLedGetProperties_t                                   pfnGetProperties;
    zes_pfnLedGetState_t                                        pfnGetState;
    zes_pfnLedSetState_t                                        pfnSetState;
    zes_pfnLedSetColor_t                                        pfnSetColor;
} zes_led_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Ras functions pointers
typedef struct _zes_ras_dditable_t_1_0
{
    zes_pfnRasGetProperties_t                                   pfnGetProperties;
    zes_pfnRasGetConfig_t                                       pfnGetConfig;
    zes_pfnRasSetConfig_t                                       pfnSetConfig;
    zes_pfnRasGetState_t                                        pfnGetState;
} zes_ras_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of RasExp functions pointers
typedef struct _zes_ras_exp_dditable_t_1_0
{
    zes_pfnRasGetStateExp_t                                     pfnGetStateExp;
    zes_pfnRasClearStateExp_t                                   pfnClearStateExp;
} zes_ras_exp_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Diagnostics functions pointers
typedef struct _zes_diagnostics_dditable_t_1_0
{
    zes_pfnDiagnosticsGetProperties_t                           pfnGetProperties;
    zes_pfnDiagnosticsGetTests_t                                pfnGetTests;
    zes_pfnDiagnosticsRunTests_t                                pfnRunTests;
} zes_diagnostics_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of VFManagementExp functions pointers
typedef struct _zes_vf_management_exp_dditable_t_1_9
{
    zes_pfnVFManagementGetVFPropertiesExp_t                     pfnGetVFPropertiesExp;
    zes_pfnVFManagementGetVFMemoryUtilizationExp_t              pfnGetVFMemoryUtilizationExp;
    zes_pfnVFManagementGetVFEngineUtilizationExp_t              pfnGetVFEngineUtilizationExp;
    zes_pfnVFManagementSetVFTelemetryModeExp_t                  pfnSetVFTelemetryModeExp;
    zes_pfnVFManagementSetVFTelemetrySamplingIntervalExp_t      pfnSetVFTelemetrySamplingIntervalExp;
} zes_vf_management_exp_dditable_t_1_9;


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZES_DDI_VER_H
