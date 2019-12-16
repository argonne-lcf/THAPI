/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_sysman.h
 *
 * @brief Intel 'One API' Level-Zero Tool APIs for System Resource Management (SMI)
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/sysman.yml
 * @endcond
 *
 */
#ifndef _ZET_SYSMAN_H
#define _ZET_SYSMAN_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _zet_sysman_version_t
{
    ZET_SYSMAN_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_sysman_version_t;

ze_result_t __zecall
zetSysmanGet(
    zet_device_handle_t hDevice,
    zet_sysman_version_t version,
    zet_sysman_handle_t* phSysman
    );

#ifndef ZET_STRING_PROPERTY_SIZE
#define ZET_STRING_PROPERTY_SIZE  32
#endif // ZET_STRING_PROPERTY_SIZE

typedef enum _zet_engine_type_t
{
    ZET_ENGINE_TYPE_OTHER = 0,
    ZET_ENGINE_TYPE_COMPUTE,
    ZET_ENGINE_TYPE_3D,
    ZET_ENGINE_TYPE_MEDIA,
    ZET_ENGINE_TYPE_DMA,

} zet_engine_type_t;

typedef struct _zet_sysman_properties_t
{
    ze_device_properties_t core;
    uint32_t numSubdevices;
    int8_t serialNumber[ZET_STRING_PROPERTY_SIZE];
    int8_t boardNumber[ZET_STRING_PROPERTY_SIZE];
    int8_t brandName[ZET_STRING_PROPERTY_SIZE];
    int8_t modelName[ZET_STRING_PROPERTY_SIZE];
    int8_t vendorName[ZET_STRING_PROPERTY_SIZE];
    int8_t driverVersion[ZET_STRING_PROPERTY_SIZE];

} zet_sysman_properties_t;

ze_result_t __zecall
zetSysmanDeviceGetProperties(
    zet_sysman_handle_t hSysman,
    zet_sysman_properties_t* pProperties
    );

typedef enum _zet_sched_mode_t
{
    ZET_SCHED_MODE_TIMEOUT = 0,
    ZET_SCHED_MODE_TIMESLICE,
    ZET_SCHED_MODE_EXCLUSIVE,
    ZET_SCHED_MODE_COMPUTE_UNIT_DEBUG,

} zet_sched_mode_t;

#ifndef ZET_SCHED_WATCHDOG_DISABLE
#define ZET_SCHED_WATCHDOG_DISABLE  (~(0ULL))
#endif // ZET_SCHED_WATCHDOG_DISABLE

typedef struct _zet_sched_timeout_properties_t
{
    uint64_t watchdogTimeout;

} zet_sched_timeout_properties_t;

typedef struct _zet_sched_timeslice_properties_t
{
    uint64_t interval;
    uint64_t yieldTimeout;

} zet_sched_timeslice_properties_t;

ze_result_t __zecall
zetSysmanSchedulerGetCurrentMode(
    zet_sysman_handle_t hSysman,
    zet_sched_mode_t* pMode
    );

ze_result_t __zecall
zetSysmanSchedulerGetTimeoutModeProperties(
    zet_sysman_handle_t hSysman,
    ze_bool_t getDefaults,
    zet_sched_timeout_properties_t* pConfig
    );

ze_result_t __zecall
zetSysmanSchedulerGetTimesliceModeProperties(
    zet_sysman_handle_t hSysman,
    ze_bool_t getDefaults,
    zet_sched_timeslice_properties_t* pConfig
    );

ze_result_t __zecall
zetSysmanSchedulerSetTimeoutMode(
    zet_sysman_handle_t hSysman,
    zet_sched_timeout_properties_t* pProperties,
    ze_bool_t* pNeedReboot
    );

ze_result_t __zecall
zetSysmanSchedulerSetTimesliceMode(
    zet_sysman_handle_t hSysman,
    zet_sched_timeslice_properties_t* pProperties,
    ze_bool_t* pNeedReboot
    );

ze_result_t __zecall
zetSysmanSchedulerSetExclusiveMode(
    zet_sysman_handle_t hSysman,
    ze_bool_t* pNeedReboot
    );

ze_result_t __zecall
zetSysmanSchedulerSetComputeUnitDebugMode(
    zet_sysman_handle_t hSysman,
    ze_bool_t* pNeedReboot
    );

typedef struct _zet_process_state_t
{
    uint32_t processId;
    int64_t memSize;
    int64_t engines;

} zet_process_state_t;

ze_result_t __zecall
zetSysmanProcessesGetState(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_process_state_t* pProcesses
    );

ze_result_t __zecall
zetSysmanDeviceReset(
    zet_sysman_handle_t hSysman
    );

ze_result_t __zecall
zetSysmanDeviceWasRepaired(
    zet_sysman_handle_t hSysman,
    ze_bool_t* pWasRepaired
    );

typedef struct _zet_pci_address_t
{
    uint32_t domain;
    uint32_t bus;
    uint32_t device;
    uint32_t function;

} zet_pci_address_t;

typedef struct _zet_pci_speed_t
{
    uint32_t gen;
    uint32_t width;
    uint64_t maxBandwidth;
    uint32_t maxPacketSize;

} zet_pci_speed_t;

typedef struct _zet_pci_properties_t
{
    zet_pci_address_t address;
    uint32_t numBars;
    zet_pci_speed_t maxSpeed;

} zet_pci_properties_t;

typedef struct _zet_pci_state_t
{
    zet_pci_speed_t speed;

} zet_pci_state_t;

typedef enum _zet_pci_bar_type_t
{
    ZET_PCI_BAR_TYPE_CONFIG = 0,
    ZET_PCI_BAR_TYPE_MMIO,
    ZET_PCI_BAR_TYPE_VRAM,
    ZET_PCI_BAR_TYPE_ROM,
    ZET_PCI_BAR_TYPE_VGA_IO,
    ZET_PCI_BAR_TYPE_VGA_MEM,
    ZET_PCI_BAR_TYPE_INDIRECT_IO,
    ZET_PCI_BAR_TYPE_INDIRECT_MEM,
    ZET_PCI_BAR_TYPE_OTHER,

} zet_pci_bar_type_t;

typedef struct _zet_pci_bar_properties_t
{
    zet_pci_bar_type_t type;
    uint64_t base;
    uint64_t size;

} zet_pci_bar_properties_t;

typedef struct _zet_pci_stats_t
{
    uint64_t timestamp;
    uint64_t replayCounter;
    uint64_t packetCounter;
    uint64_t rxCounter;
    uint64_t txCounter;
    uint64_t maxBandwidth;

} zet_pci_stats_t;

ze_result_t __zecall
zetSysmanPciGetProperties(
    zet_sysman_handle_t hSysman,
    zet_pci_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanPciGetState(
    zet_sysman_handle_t hSysman,
    zet_pci_state_t* pState
    );

ze_result_t __zecall
zetSysmanPciGetBarProperties(
    zet_sysman_handle_t hSysman,
    uint32_t barIndex,
    zet_pci_bar_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanPciGetStats(
    zet_sysman_handle_t hSysman,
    zet_pci_stats_t* pStats
    );

typedef struct _zet_power_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    ze_bool_t isEnergyThresholdSupported;
    uint32_t maxLimit;

} zet_power_properties_t;

typedef struct _zet_power_energy_counter_t
{
    uint64_t energy;
    uint64_t timestamp;

} zet_power_energy_counter_t;

typedef struct _zet_power_sustained_limit_t
{
    ze_bool_t enabled;
    uint32_t power;
    uint32_t interval;

} zet_power_sustained_limit_t;

typedef struct _zet_power_burst_limit_t
{
    ze_bool_t enabled;
    uint32_t power;

} zet_power_burst_limit_t;

typedef struct _zet_power_peak_limit_t
{
    uint32_t powerAC;
    uint32_t powerDC;

} zet_power_peak_limit_t;

typedef struct _zet_energy_threshold_t
{
    ze_bool_t enable;
    double threshold;
    uint32_t processId;

} zet_energy_threshold_t;

ze_result_t __zecall
zetSysmanPowerGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_pwr_handle_t* phPower
    );

ze_result_t __zecall
zetSysmanPowerGetProperties(
    zet_sysman_pwr_handle_t hPower,
    zet_power_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanPowerGetEnergyCounter(
    zet_sysman_pwr_handle_t hPower,
    zet_power_energy_counter_t* pEnergy
    );

ze_result_t __zecall
zetSysmanPowerGetLimits(
    zet_sysman_pwr_handle_t hPower,
    zet_power_sustained_limit_t* pSustained,
    zet_power_burst_limit_t* pBurst,
    zet_power_peak_limit_t* pPeak
    );

ze_result_t __zecall
zetSysmanPowerSetLimits(
    zet_sysman_pwr_handle_t hPower,
    const zet_power_sustained_limit_t* pSustained,
    const zet_power_burst_limit_t* pBurst,
    const zet_power_peak_limit_t* pPeak
    );

ze_result_t __zecall
zetSysmanPowerGetEnergyThreshold(
    zet_sysman_pwr_handle_t hPower,
    zet_energy_threshold_t* pThreshold
    );

ze_result_t __zecall
zetSysmanPowerSetEnergyThreshold(
    zet_sysman_pwr_handle_t hPower,
    double threshold
    );

typedef enum _zet_freq_domain_t
{
    ZET_FREQ_DOMAIN_GPU = 0,
    ZET_FREQ_DOMAIN_MEMORY,

} zet_freq_domain_t;

typedef struct _zet_freq_properties_t
{
    zet_freq_domain_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    double min;
    double max;
    double step;

} zet_freq_properties_t;

typedef struct _zet_freq_range_t
{
    double min;
    double max;

} zet_freq_range_t;

typedef enum _zet_freq_throttle_reasons_t
{
    ZET_FREQ_THROTTLE_REASONS_NONE = 0,
    ZET_FREQ_THROTTLE_REASONS_AVE_PWR_CAP = ZE_BIT( 0 ),
    ZET_FREQ_THROTTLE_REASONS_BURST_PWR_CAP = ZE_BIT( 1 ),
    ZET_FREQ_THROTTLE_REASONS_CURRENT_LIMIT = ZE_BIT( 2 ),
    ZET_FREQ_THROTTLE_REASONS_THERMAL_LIMIT = ZE_BIT( 3 ),
    ZET_FREQ_THROTTLE_REASONS_PSU_ALERT = ZE_BIT( 4 ),
    ZET_FREQ_THROTTLE_REASONS_SW_RANGE = ZE_BIT( 5 ),
    ZET_FREQ_THROTTLE_REASONS_HW_RANGE = ZE_BIT( 6 ),

} zet_freq_throttle_reasons_t;

typedef struct _zet_freq_state_t
{
    double request;
    double tdp;
    double efficient;
    double actual;
    uint32_t throttleReasons;

} zet_freq_state_t;

typedef struct _zet_freq_throttle_time_t
{
    uint64_t throttleTime;
    uint64_t timestamp;

} zet_freq_throttle_time_t;

typedef enum _zet_oc_mode_t
{
    ZET_OC_MODE_OFF = 0,
    ZET_OC_MODE_OFFSET,
    ZET_OC_MODE_OVERRIDE,

} zet_oc_mode_t;

typedef struct _zet_oc_capabilities_t
{
    ze_bool_t isOcSupported;
    double maxOcFrequencyLimit;
    double maxFactoryDefaultFrequency;
    double maxFactoryDefaultVoltage;
    ze_bool_t isTjMaxSupported;
    ze_bool_t isIccMaxSupported;
    ze_bool_t isVoltageOverrideSupported;
    ze_bool_t isVoltageOffsetSupported;
    ze_bool_t isHighVoltModeCapable;
    ze_bool_t isHighVoltModeEnabled;

} zet_oc_capabilities_t;

typedef struct _zet_oc_config_t
{
    zet_oc_mode_t mode;
    double frequency;
    double voltage;

} zet_oc_config_t;

ze_result_t __zecall
zetSysmanFrequencyGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_freq_handle_t* phFrequency
    );

ze_result_t __zecall
zetSysmanFrequencyGetProperties(
    zet_sysman_freq_handle_t hFrequency,
    zet_freq_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanFrequencyGetAvailableClocks(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    double* phFrequency
    );

ze_result_t __zecall
zetSysmanFrequencyGetRange(
    zet_sysman_freq_handle_t hFrequency,
    zet_freq_range_t* pLimits
    );

ze_result_t __zecall
zetSysmanFrequencySetRange(
    zet_sysman_freq_handle_t hFrequency,
    const zet_freq_range_t* pLimits
    );

ze_result_t __zecall
zetSysmanFrequencyGetState(
    zet_sysman_freq_handle_t hFrequency,
    zet_freq_state_t* pState
    );

ze_result_t __zecall
zetSysmanFrequencyGetThrottleTime(
    zet_sysman_freq_handle_t hFrequency,
    zet_freq_throttle_time_t* pThrottleTime
    );

ze_result_t __zecall
zetSysmanFrequencyGetOcCapabilities(
    zet_sysman_freq_handle_t hFrequency,
    zet_oc_capabilities_t* pOcCapabilities
    );

ze_result_t __zecall
zetSysmanFrequencyGetOcConfig(
    zet_sysman_freq_handle_t hFrequency,
    zet_oc_config_t* pOcConfiguration
    );

ze_result_t __zecall
zetSysmanFrequencySetOcConfig(
    zet_sysman_freq_handle_t hFrequency,
    zet_oc_config_t* pOcConfiguration
    );

ze_result_t __zecall
zetSysmanFrequencyGetOcIccMax(
    zet_sysman_freq_handle_t hFrequency,
    double* pOcIccMax
    );

ze_result_t __zecall
zetSysmanFrequencySetOcIccMax(
    zet_sysman_freq_handle_t hFrequency,
    double ocIccMax
    );

ze_result_t __zecall
zetSysmanFrequencyGetOcTjMax(
    zet_sysman_freq_handle_t hFrequency,
    double* pOcTjMax
    );

ze_result_t __zecall
zetSysmanFrequencySetOcTjMax(
    zet_sysman_freq_handle_t hFrequency,
    double ocTjMax
    );

typedef enum _zet_engine_group_t
{
    ZET_ENGINE_GROUP_ALL = 0,
    ZET_ENGINE_GROUP_COMPUTE_ALL,
    ZET_ENGINE_GROUP_MEDIA_ALL,
    ZET_ENGINE_GROUP_COPY_ALL,

} zet_engine_group_t;

typedef struct _zet_engine_properties_t
{
    zet_engine_group_t type;
    int64_t engines;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;

} zet_engine_properties_t;

typedef struct _zet_engine_stats_t
{
    uint64_t activeTime;
    uint64_t timestamp;

} zet_engine_stats_t;

ze_result_t __zecall
zetSysmanEngineGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_engine_handle_t* phEngine
    );

ze_result_t __zecall
zetSysmanEngineGetProperties(
    zet_sysman_engine_handle_t hEngine,
    zet_engine_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanEngineGetActivity(
    zet_sysman_engine_handle_t hEngine,
    zet_engine_stats_t* pStats
    );

typedef enum _zet_standby_type_t
{
    ZET_STANDBY_TYPE_GLOBAL = 0,

} zet_standby_type_t;

typedef struct _zet_standby_properties_t
{
    zet_standby_type_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;

} zet_standby_properties_t;

typedef enum _zet_standby_promo_mode_t
{
    ZET_STANDBY_PROMO_MODE_DEFAULT = 0,
    ZET_STANDBY_PROMO_MODE_NEVER,

} zet_standby_promo_mode_t;

ze_result_t __zecall
zetSysmanStandbyGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_standby_handle_t* phStandby
    );

ze_result_t __zecall
zetSysmanStandbyGetProperties(
    zet_sysman_standby_handle_t hStandby,
    zet_standby_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanStandbyGetMode(
    zet_sysman_standby_handle_t hStandby,
    zet_standby_promo_mode_t* pMode
    );

ze_result_t __zecall
zetSysmanStandbySetMode(
    zet_sysman_standby_handle_t hStandby,
    zet_standby_promo_mode_t mode
    );

typedef struct _zet_firmware_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    int8_t name[ZET_STRING_PROPERTY_SIZE];
    int8_t version[ZET_STRING_PROPERTY_SIZE];

} zet_firmware_properties_t;

ze_result_t __zecall
zetSysmanFirmwareGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_firmware_handle_t* phFirmware
    );

ze_result_t __zecall
zetSysmanFirmwareGetProperties(
    zet_sysman_firmware_handle_t hFirmware,
    zet_firmware_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanFirmwareGetChecksum(
    zet_sysman_firmware_handle_t hFirmware,
    uint32_t* pChecksum
    );

ze_result_t __zecall
zetSysmanFirmwareFlash(
    zet_sysman_firmware_handle_t hFirmware,
    void* pImage,
    uint32_t size
    );

typedef enum _zet_mem_type_t
{
    ZET_MEM_TYPE_HBM = 0,
    ZET_MEM_TYPE_DDR,
    ZET_MEM_TYPE_SRAM,
    ZET_MEM_TYPE_L1,
    ZET_MEM_TYPE_L3,
    ZET_MEM_TYPE_GRF,
    ZET_MEM_TYPE_SLM,

} zet_mem_type_t;

typedef enum _zet_mem_health_t
{
    ZET_MEM_HEALTH_OK = 0,
    ZET_MEM_HEALTH_DEGRADED,
    ZET_MEM_HEALTH_CRITICAL,
    ZET_MEM_HEALTH_REPLACE,

} zet_mem_health_t;

typedef struct _zet_mem_properties_t
{
    zet_mem_type_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    uint64_t physicalSize;

} zet_mem_properties_t;

typedef struct _zet_mem_state_t
{
    zet_mem_health_t health;
    uint64_t allocatedSize;
    uint64_t maxSize;

} zet_mem_state_t;

typedef struct _zet_mem_bandwidth_t
{
    uint64_t readCounter;
    uint64_t writeCounter;
    uint64_t maxBandwidth;
    uint64_t timestamp;

} zet_mem_bandwidth_t;

ze_result_t __zecall
zetSysmanMemoryGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_mem_handle_t* phMemory
    );

ze_result_t __zecall
zetSysmanMemoryGetProperties(
    zet_sysman_mem_handle_t hMemory,
    zet_mem_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanMemoryGetState(
    zet_sysman_mem_handle_t hMemory,
    zet_mem_state_t* pState
    );

ze_result_t __zecall
zetSysmanMemoryGetBandwidth(
    zet_sysman_mem_handle_t hMemory,
    zet_mem_bandwidth_t* pBandwidth
    );

#ifndef ZET_MAX_FABRIC_PORT_MODEL_SIZE
#define ZET_MAX_FABRIC_PORT_MODEL_SIZE  256
#endif // ZET_MAX_FABRIC_PORT_MODEL_SIZE

#ifndef ZET_MAX_FABRIC_PORT_UUID_SIZE
#define ZET_MAX_FABRIC_PORT_UUID_SIZE  72
#endif // ZET_MAX_FABRIC_PORT_UUID_SIZE

#ifndef ZET_MAX_FABRIC_LINK_TYPE_SIZE
#define ZET_MAX_FABRIC_LINK_TYPE_SIZE  256
#endif // ZET_MAX_FABRIC_LINK_TYPE_SIZE

typedef enum _zet_fabric_port_status_t
{
    ZET_FABRIC_PORT_STATUS_GREEN = 0,
    ZET_FABRIC_PORT_STATUS_YELLOW,
    ZET_FABRIC_PORT_STATUS_RED,
    ZET_FABRIC_PORT_STATUS_BLACK,

} zet_fabric_port_status_t;

typedef enum _zet_fabric_port_qual_issues_t
{
    ZET_FABRIC_PORT_QUAL_ISSUES_NONE = 0,
    ZET_FABRIC_PORT_QUAL_ISSUES_FEC = ZE_BIT( 0 ),
    ZET_FABRIC_PORT_QUAL_ISSUES_LTP_CRC = ZE_BIT( 1 ),
    ZET_FABRIC_PORT_QUAL_ISSUES_SPEED = ZE_BIT( 2 ),

} zet_fabric_port_qual_issues_t;

typedef enum _zet_fabric_port_stab_issues_t
{
    ZET_FABRIC_PORT_STAB_ISSUES_NONE = 0,
    ZET_FABRIC_PORT_STAB_ISSUES_TOO_MANY_REPLAYS = ZE_BIT( 0 ),
    ZET_FABRIC_PORT_STAB_ISSUES_NO_CONNECT = ZE_BIT( 1 ),
    ZET_FABRIC_PORT_STAB_ISSUES_FLAPPING = ZE_BIT( 2 ),

} zet_fabric_port_stab_issues_t;

typedef struct _zet_fabric_port_uuid_t
{
    uint8_t id[ZET_MAX_FABRIC_PORT_UUID_SIZE];

} zet_fabric_port_uuid_t;

typedef struct _zet_fabric_port_speed_t
{
    uint64_t bitRate;
    uint32_t width;
    uint64_t maxBandwidth;

} zet_fabric_port_speed_t;

typedef struct _zet_fabric_port_properties_t
{
    int8_t model[ZET_MAX_FABRIC_PORT_MODEL_SIZE];
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    zet_fabric_port_uuid_t portUuid;
    zet_fabric_port_speed_t maxRxSpeed;
    zet_fabric_port_speed_t maxTxSpeed;

} zet_fabric_port_properties_t;

typedef struct _zet_fabric_link_type_t
{
    int8_t desc[ZET_MAX_FABRIC_LINK_TYPE_SIZE];

} zet_fabric_link_type_t;

typedef struct _zet_fabric_port_config_t
{
    ze_bool_t enabled;
    ze_bool_t beaconing;

} zet_fabric_port_config_t;

typedef struct _zet_fabric_port_state_t
{
    zet_fabric_port_status_t status;
    zet_fabric_port_qual_issues_t qualityIssues;
    zet_fabric_port_stab_issues_t stabilityIssues;
    zet_fabric_port_speed_t rxSpeed;
    zet_fabric_port_speed_t txSpeed;

} zet_fabric_port_state_t;

typedef struct _zet_fabric_port_throughput_t
{
    uint64_t timestamp;
    uint64_t rxCounter;
    uint64_t txCounter;
    uint64_t rxMaxBandwidth;
    uint64_t txMaxBandwidth;

} zet_fabric_port_throughput_t;

ze_result_t __zecall
zetSysmanFabricPortGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_fabric_port_handle_t* phPort
    );

ze_result_t __zecall
zetSysmanFabricPortGetProperties(
    zet_sysman_fabric_port_handle_t hPort,
    zet_fabric_port_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanFabricPortGetLinkType(
    zet_sysman_fabric_port_handle_t hPort,
    ze_bool_t verbose,
    zet_fabric_link_type_t* pLinkType
    );

ze_result_t __zecall
zetSysmanFabricPortGetConfig(
    zet_sysman_fabric_port_handle_t hPort,
    zet_fabric_port_config_t* pConfig
    );

ze_result_t __zecall
zetSysmanFabricPortSetConfig(
    zet_sysman_fabric_port_handle_t hPort,
    zet_fabric_port_config_t* pConfig
    );

ze_result_t __zecall
zetSysmanFabricPortGetState(
    zet_sysman_fabric_port_handle_t hPort,
    zet_fabric_port_state_t* pState
    );

ze_result_t __zecall
zetSysmanFabricPortGetThroughput(
    zet_sysman_fabric_port_handle_t hPort,
    zet_fabric_port_throughput_t* pThroughput
    );

typedef enum _zet_temp_sensors_t
{
    ZET_TEMP_SENSORS_GLOBAL = 0,
    ZET_TEMP_SENSORS_GPU,
    ZET_TEMP_SENSORS_MEMORY,

} zet_temp_sensors_t;

typedef struct _zet_temp_properties_t
{
    zet_temp_sensors_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t isThreshold1Supported;
    ze_bool_t isThreshold2Supported;

} zet_temp_properties_t;

typedef struct _zet_temp_threshold_t
{
    ze_bool_t enableLowToHigh;
    ze_bool_t enableHighToLow;
    ze_bool_t threshold;
    uint32_t processId;

} zet_temp_threshold_t;

ze_result_t __zecall
zetSysmanTemperatureRead(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_temp_handle_t* phTemperature
    );

ze_result_t __zecall
zetSysmanTemperatureGetProperties(
    zet_sysman_temp_handle_t hTemperature,
    zet_temp_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanTemperatureGet(
    zet_sysman_temp_handle_t hTemperature,
    double* pTemperature
    );

ze_result_t __zecall
zetSysmanTemperatureGetThresholds(
    zet_sysman_temp_handle_t hTemperature,
    zet_temp_threshold_t* pThreshold1,
    zet_temp_threshold_t* pThreshold2
    );

ze_result_t __zecall
zetSysmanTemperatureSetThresholds(
    zet_sysman_temp_handle_t hTemperature,
    double threshold1,
    double threshold2
    );

typedef enum _zet_psu_voltage_status_t
{
    ZET_PSU_VOLTAGE_STATUS_NORMAL = 0,
    ZET_PSU_VOLTAGE_STATUS_OVER,
    ZET_PSU_VOLTAGE_STATUS_UNDER,

} zet_psu_voltage_status_t;

typedef struct _zet_psu_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    ze_bool_t haveFan;
    uint32_t ampLimit;

} zet_psu_properties_t;

typedef struct _zet_psu_state_t
{
    zet_psu_voltage_status_t voltStatus;
    ze_bool_t fanFailed;
    uint32_t temperature;
    uint32_t current;

} zet_psu_state_t;

ze_result_t __zecall
zetSysmanPsuGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_psu_handle_t* phPsu
    );

ze_result_t __zecall
zetSysmanPsuGetProperties(
    zet_sysman_psu_handle_t hPsu,
    zet_psu_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanPsuGetState(
    zet_sysman_psu_handle_t hPsu,
    zet_psu_state_t* pState
    );

typedef enum _zet_fan_speed_mode_t
{
    ZET_FAN_SPEED_MODE_DEFAULT = 0,
    ZET_FAN_SPEED_MODE_FIXED,
    ZET_FAN_SPEED_MODE_TABLE,

} zet_fan_speed_mode_t;

typedef enum _zet_fan_speed_units_t
{
    ZET_FAN_SPEED_UNITS_RPM = 0,
    ZET_FAN_SPEED_UNITS_PERCENT,

} zet_fan_speed_units_t;

typedef struct _zet_fan_temp_speed_t
{
    uint32_t temperature;
    uint32_t speed;
    zet_fan_speed_units_t units;

} zet_fan_temp_speed_t;

#ifndef ZET_FAN_TEMP_SPEED_PAIR_COUNT
#define ZET_FAN_TEMP_SPEED_PAIR_COUNT  32
#endif // ZET_FAN_TEMP_SPEED_PAIR_COUNT

typedef struct _zet_fan_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    uint32_t maxSpeed;
    uint32_t maxPoints;

} zet_fan_properties_t;

typedef struct _zet_fan_config_t
{
    zet_fan_speed_mode_t mode;
    uint32_t speed;
    zet_fan_speed_units_t speedUnits;
    uint32_t numPoints;
    zet_fan_temp_speed_t table[ZET_FAN_TEMP_SPEED_PAIR_COUNT];

} zet_fan_config_t;

typedef struct _zet_fan_state_t
{
    zet_fan_speed_mode_t mode;
    zet_fan_speed_units_t speedUnits;
    uint32_t speed;

} zet_fan_state_t;

ze_result_t __zecall
zetSysmanFanGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_fan_handle_t* phFan
    );

ze_result_t __zecall
zetSysmanFanGetProperties(
    zet_sysman_fan_handle_t hFan,
    zet_fan_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanFanGetConfig(
    zet_sysman_fan_handle_t hFan,
    zet_fan_config_t* pConfig
    );

ze_result_t __zecall
zetSysmanFanSetConfig(
    zet_sysman_fan_handle_t hFan,
    const zet_fan_config_t* pConfig
    );

ze_result_t __zecall
zetSysmanFanGetState(
    zet_sysman_fan_handle_t hFan,
    zet_fan_speed_units_t units,
    zet_fan_state_t* pState
    );

typedef struct _zet_led_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    ze_bool_t haveRGB;

} zet_led_properties_t;

typedef struct _zet_led_state_t
{
    ze_bool_t isOn;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

} zet_led_state_t;

ze_result_t __zecall
zetSysmanLedGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_led_handle_t* phLed
    );

ze_result_t __zecall
zetSysmanLedGetProperties(
    zet_sysman_led_handle_t hLed,
    zet_led_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanLedGetState(
    zet_sysman_led_handle_t hLed,
    zet_led_state_t* pState
    );

ze_result_t __zecall
zetSysmanLedSetState(
    zet_sysman_led_handle_t hLed,
    const zet_led_state_t* pState
    );

typedef enum _zet_ras_error_type_t
{
    ZET_RAS_ERROR_TYPE_CORRECTABLE = 0,
    ZET_RAS_ERROR_TYPE_UNCORRECTABLE,

} zet_ras_error_type_t;

typedef struct _zet_ras_properties_t
{
    zet_ras_error_type_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t supported;
    ze_bool_t enabled;

} zet_ras_properties_t;

typedef struct _zet_ras_details_t
{
    uint64_t numResets;
    uint64_t numProgrammingErrors;
    uint64_t numDriverErrors;
    uint64_t numComputeErrors;
    uint64_t numNonComputeErrors;
    uint64_t numCacheErrors;
    uint64_t numMemoryErrors;
    uint64_t numPciErrors;
    uint64_t numFabricErrors;
    uint64_t numDisplayErrors;

} zet_ras_details_t;

ze_result_t __zecall
zetSysmanRasGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_ras_handle_t* phRas
    );

ze_result_t __zecall
zetSysmanRasGetProperties(
    zet_sysman_ras_handle_t hRas,
    zet_ras_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanRasGetErrors(
    zet_sysman_ras_handle_t hRas,
    ze_bool_t clear,
    uint64_t* pTotalErrors,
    zet_ras_details_t* pDetails
    );

typedef enum _zet_sysman_event_type_t
{
    ZET_SYSMAN_EVENT_TYPE_FREQ_THROTTLED = 0,
    ZET_SYSMAN_EVENT_TYPE_ENERGY_THRESHOLD_CROSSED,
    ZET_SYSMAN_EVENT_TYPE_CRITICAL_TEMPERATURE,
    ZET_SYSMAN_EVENT_TYPE_TEMP_THRESHOLD1,
    ZET_SYSMAN_EVENT_TYPE_TEMP_THRESHOLD2,
    ZET_SYSMAN_EVENT_TYPE_RAS_ERRORS,
    ZET_SYSMAN_EVENT_TYPE_NUM,

} zet_sysman_event_type_t;

typedef struct _zet_event_properties_t
{
    ze_bool_t supportedEvents[ZET_SYSMAN_EVENT_TYPE_NUM];

} zet_event_properties_t;

typedef struct _zet_event_request_t
{
    zet_sysman_event_type_t event;
    uint32_t threshold;

} zet_event_request_t;

ze_result_t __zecall
zetSysmanEventsGetProperties(
    zet_sysman_handle_t hSysman,
    zet_event_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanEventsRegister(
    zet_sysman_handle_t hSysman,
    uint32_t count,
    zet_event_request_t* pEvents
    );

ze_result_t __zecall
zetSysmanEventsUnregister(
    zet_sysman_handle_t hSysman,
    uint32_t count,
    zet_event_request_t* pEvents
    );

#ifndef ZET_EVENT_WAIT_INFINITE
#define ZET_EVENT_WAIT_INFINITE  0xFFFFFFFF
#endif // ZET_EVENT_WAIT_INFINITE

ze_result_t __zecall
zetSysmanEventsListen(
    zet_sysman_handle_t hSysman,
    ze_bool_t clear,
    uint32_t timeout,
    uint32_t* pEvents
    );

typedef enum _zet_diag_type_t
{
    ZET_DIAG_TYPE_SCAN = 0,
    ZET_DIAG_TYPE_ARRAY,

} zet_diag_type_t;

typedef enum _zet_diag_result_t
{
    ZET_DIAG_RESULT_NO_ERRORS = 0,
    ZET_DIAG_RESULT_ABORT,
    ZET_DIAG_RESULT_FAIL_CANT_REPAIR,
    ZET_DIAG_RESULT_REBOOT_FOR_REPAIR,

} zet_diag_result_t;

#ifndef ZET_DIAG_FIRST_TEST_INDEX
#define ZET_DIAG_FIRST_TEST_INDEX  0x0
#endif // ZET_DIAG_FIRST_TEST_INDEX

#ifndef ZET_DIAG_LAST_TEST_INDEX
#define ZET_DIAG_LAST_TEST_INDEX  0xFFFFFFFF
#endif // ZET_DIAG_LAST_TEST_INDEX

typedef struct _zet_diag_test_t
{
    uint32_t index;
    const char* name;

} zet_diag_test_t;

typedef struct _zet_diag_properties_t
{
    zet_diag_type_t type;
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    const char* name;
    uint32_t numTests;
    const zet_diag_test_t* pTests;

} zet_diag_properties_t;

ze_result_t __zecall
zetSysmanDiagnosticsGet(
    zet_sysman_handle_t hSysman,
    uint32_t* pCount,
    zet_sysman_diag_handle_t* phDiagnostics
    );

ze_result_t __zecall
zetSysmanDiagnosticsGetProperties(
    zet_sysman_diag_handle_t hDiagnostics,
    zet_diag_properties_t* pProperties
    );

ze_result_t __zecall
zetSysmanDiagnosticsRunTests(
    zet_sysman_diag_handle_t hDiagnostics,
    uint32_t start,
    uint32_t end,
    zet_diag_result_t* pResult
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_SYSMAN_H
