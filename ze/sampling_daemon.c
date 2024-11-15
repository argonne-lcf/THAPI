#include "thapi_sampling.h"
#include "ze_build.h"
#include "ze_sampling.h"
#include <dlfcn.h>
#include <errno.h>
#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#define SIG_SAMPLING_READY (SIGRTMIN)
#define SIG_SAMPLING_FINISH (SIGRTMIN + 1)

#define ZES_INIT_PTR zesInit_ptr
#define ZES_DRIVER_GET_PTR zesDriverGet_ptr
#define ZES_DEVICE_GET_PTR zesDeviceGet_ptr
#define ZES_DEVICE_GET_PROPERTIES_PTR zesDeviceGetProperties_ptr
#define ZES_DEVICE_ENUM_POWER_DOMAINS_PTR zesDeviceEnumPowerDomains_ptr
#define ZES_POWER_GET_PROPERTIES_PTR zesPowerGetProperties_ptr
#define ZES_POWER_GET_ENERGY_COUNTER_PTR zesPowerGetEnergyCounter_ptr
#define ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR zesDeviceEnumFrequencyDomains_ptr
#define ZES_FREQUENCY_GET_PROPERTIES_PTR zesFrequencyGetProperties_ptr
#define ZES_FREQUENCY_GET_STATE_PTR zesFrequencyGetState_ptr
#define ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR zesDeviceEnumEngineGroups_ptr
#define ZES_ENGINE_GET_PROPERTIES_PTR zesEngineGetProperties_ptr
#define ZES_ENGINE_GET_ACTIVITY_PTR zesEngineGetActivity_ptr
#define ZES_DEVICE_ENUM_FABRIC_PORTS_PTR zesDeviceEnumFabricPorts_ptr
#define ZES_FABRIC_PORT_GET_PROPERTIES_PTR zesFabricPortGetProperties_ptr
#define ZES_FABRIC_PORT_GET_STATE_PTR zesFabricPortGetState_ptr
#define ZES_FABRIC_PORT_GET_THROUGHPUT_PTR zesFabricPortGetThroughput_ptr
#define ZES_DEVICE_ENUM_MEMORY_MODULES_PTR zesDeviceEnumMemoryModules_ptr
#define ZES_MEMORY_GET_PROPERTIES_PTR zesMemoryGetProperties_ptr
#define ZES_MEMORY_GET_STATE_PTR zesMemoryGetState_ptr
#define ZES_MEMORY_GET_BANDWIDTH_PTR zesMemoryGetBandwidth_ptr

typedef ze_result_t (*zesInit_t)(zes_init_flags_t flags);
static zesInit_t ZES_INIT_PTR = (void *)0x0;

typedef ze_result_t (*zesDriverGet_t)(uint32_t *pCount, zes_driver_handle_t *phDrivers);
static zesDriverGet_t ZES_DRIVER_GET_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceGet_t)(zes_driver_handle_t hDriver, uint32_t *pCount,
                                      zes_device_handle_t *phDevices);
static zesDeviceGet_t ZES_DEVICE_GET_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceGetProperties_t)(zes_device_handle_t hDevice,
                                                zes_device_properties_t *pProperties);
static zesDeviceGetProperties_t ZES_DEVICE_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceEnumPowerDomains_t)(zes_device_handle_t hDevice, uint32_t *pCount,
                                                   zes_pwr_handle_t *phPower);
static zesDeviceEnumPowerDomains_t ZES_DEVICE_ENUM_POWER_DOMAINS_PTR = (void *)0x0;

typedef ze_result_t (*zesPowerGetProperties_t)(zes_pwr_handle_t hPower,
                                               zes_power_properties_t *pProperties);
static zesPowerGetProperties_t ZES_POWER_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesPowerGetEnergyCounter_t)(zes_pwr_handle_t hPower,
                                                  zes_power_energy_counter_t *pEnergy);
static zesPowerGetEnergyCounter_t ZES_POWER_GET_ENERGY_COUNTER_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceEnumFrequencyDomains_t)(zes_device_handle_t hDevice,
                                                       uint32_t *pCount,
                                                       zes_freq_handle_t *phFrequency);
static zesDeviceEnumFrequencyDomains_t ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR = (void *)0x0;

typedef ze_result_t (*zesFrequencyGetProperties_t)(zes_freq_handle_t hFrequency,
                                                   zes_freq_properties_t *pProperties);
static zesFrequencyGetProperties_t ZES_FREQUENCY_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesFrequencyGetState_t)(zes_freq_handle_t hFrequency,
                                              zes_freq_state_t *pState);
static zesFrequencyGetState_t ZES_FREQUENCY_GET_STATE_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceEnumEngineGroups_t)(zes_device_handle_t hDevice, uint32_t *pCount,
                                                   zes_engine_handle_t *phEngine);
static zesDeviceEnumEngineGroups_t ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR = (void *)0x0;

typedef ze_result_t (*zesEngineGetProperties_t)(zes_engine_handle_t hEngine,
                                                zes_engine_properties_t *pProperties);
static zesEngineGetProperties_t ZES_ENGINE_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesEngineGetActivity_t)(zes_engine_handle_t hEngine,
                                              zes_engine_stats_t *pStats);
static zesEngineGetActivity_t ZES_ENGINE_GET_ACTIVITY_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceEnumFabricPorts_t)(zes_device_handle_t hDevice, uint32_t *pCount,
                                                  zes_fabric_port_handle_t *phPort);
static zesDeviceEnumFabricPorts_t ZES_DEVICE_ENUM_FABRIC_PORTS_PTR = (void *)0x0;

typedef ze_result_t (*zesFabricPortGetProperties_t)(zes_fabric_port_handle_t hPort,
                                                    zes_fabric_port_properties_t *pProperties);
static zesFabricPortGetProperties_t ZES_FABRIC_PORT_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesFabricPortGetState_t)(zes_fabric_port_handle_t hPort,
                                               zes_fabric_port_state_t *pState);
static zesFabricPortGetState_t ZES_FABRIC_PORT_GET_STATE_PTR = (void *)0x0;

typedef ze_result_t (*zesFabricPortGetThroughput_t)(zes_fabric_port_handle_t hPort,
                                                    zes_fabric_port_throughput_t *pThroughput);
static zesFabricPortGetThroughput_t ZES_FABRIC_PORT_GET_THROUGHPUT_PTR = (void *)0x0;

typedef ze_result_t (*zesDeviceEnumMemoryModules_t)(zes_device_handle_t hDevice, uint32_t *pCount,
                                                    zes_mem_handle_t *phMemory);
static zesDeviceEnumMemoryModules_t ZES_DEVICE_ENUM_MEMORY_MODULES_PTR = (void *)0x0;

typedef ze_result_t (*zesMemoryGetProperties_t)(zes_mem_handle_t hMemory,
                                                zes_mem_properties_t *pProperties);
static zesMemoryGetProperties_t ZES_MEMORY_GET_PROPERTIES_PTR = (void *)0x0;

typedef ze_result_t (*zesMemoryGetState_t)(zes_mem_handle_t hMemory, zes_mem_state_t *pState);
static zesMemoryGetState_t ZES_MEMORY_GET_STATE_PTR = (void *)0x0;

typedef ze_result_t (*zesMemoryGetBandwidth_t)(zes_mem_handle_t hMemory,
                                               zes_mem_bandwidth_t *pBandwidth);
static zesMemoryGetBandwidth_t ZES_MEMORY_GET_BANDWIDTH_PTR = (void *)0x0;

static void find_ze_symbols(void *handle, int verbose) {

  ZES_INIT_PTR = (zesInit_t)(intptr_t)dlsym(handle, "zesInit");
  if (!ZES_INIT_PTR && verbose)
    fprintf(stderr, "Missing symbol zesInit!\n");

  ZES_DRIVER_GET_PTR = (zesDriverGet_t)(intptr_t)dlsym(handle, "zesDriverGet");
  if (!ZES_DRIVER_GET_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDriverGet!\n");

  ZES_DEVICE_GET_PTR = (zesDeviceGet_t)(intptr_t)dlsym(handle, "zesDeviceGet");
  if (!ZES_DEVICE_GET_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceGet!\n");

  ZES_DEVICE_GET_PROPERTIES_PTR =
      (zesDeviceGetProperties_t)(intptr_t)dlsym(handle, "zesDeviceGetProperties");
  if (!ZES_DEVICE_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceGetProperties!\n");

  ZES_DEVICE_ENUM_POWER_DOMAINS_PTR =
      (zesDeviceEnumPowerDomains_t)(intptr_t)dlsym(handle, "zesDeviceEnumPowerDomains");
  if (!ZES_DEVICE_ENUM_POWER_DOMAINS_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceEnumPowerDomains!\n");

  ZES_POWER_GET_PROPERTIES_PTR =
      (zesPowerGetProperties_t)(intptr_t)dlsym(handle, "zesPowerGetProperties");
  if (!ZES_POWER_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesPowerGetProperties!\n");

  ZES_POWER_GET_ENERGY_COUNTER_PTR =
      (zesPowerGetEnergyCounter_t)(intptr_t)dlsym(handle, "zesPowerGetEnergyCounter");
  if (!ZES_POWER_GET_ENERGY_COUNTER_PTR && verbose)
    fprintf(stderr, "Missing symbol zesPowerGetEnergyCounter!\n");

  ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR =
      (zesDeviceEnumFrequencyDomains_t)(intptr_t)dlsym(handle, "zesDeviceEnumFrequencyDomains");
  if (!ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceEnumFrequencyDomains!\n");

  ZES_FREQUENCY_GET_PROPERTIES_PTR =
      (zesFrequencyGetProperties_t)(intptr_t)dlsym(handle, "zesFrequencyGetProperties");
  if (!ZES_FREQUENCY_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesFrequencyGetProperties!\n");

  ZES_FREQUENCY_GET_STATE_PTR =
      (zesFrequencyGetState_t)(intptr_t)dlsym(handle, "zesFrequencyGetState");
  if (!ZES_FREQUENCY_GET_STATE_PTR && verbose)
    fprintf(stderr, "Missing symbol zesFrequencyGetState!\n");

  ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR =
      (zesDeviceEnumEngineGroups_t)(intptr_t)dlsym(handle, "zesDeviceEnumEngineGroups");
  if (!ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceEnumEngineGroups!\n");

  ZES_ENGINE_GET_PROPERTIES_PTR =
      (zesEngineGetProperties_t)(intptr_t)dlsym(handle, "zesEngineGetProperties");
  if (!ZES_ENGINE_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesEngineGetProperties!\n");

  ZES_ENGINE_GET_ACTIVITY_PTR =
      (zesEngineGetActivity_t)(intptr_t)dlsym(handle, "zesEngineGetActivity");
  if (!ZES_ENGINE_GET_ACTIVITY_PTR && verbose)
    fprintf(stderr, "Missing symbol zesEngineGetActivity!\n");

  ZES_DEVICE_ENUM_FABRIC_PORTS_PTR =
      (zesDeviceEnumFabricPorts_t)(intptr_t)dlsym(handle, "zesDeviceEnumFabricPorts");
  if (!ZES_DEVICE_ENUM_FABRIC_PORTS_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceEnumFabricPorts!\n");

  ZES_FABRIC_PORT_GET_PROPERTIES_PTR =
      (zesFabricPortGetProperties_t)(intptr_t)dlsym(handle, "zesFabricPortGetProperties");
  if (!ZES_FABRIC_PORT_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesFabricPortGetProperties!\n");

  ZES_FABRIC_PORT_GET_STATE_PTR =
      (zesFabricPortGetState_t)(intptr_t)dlsym(handle, "zesFabricPortGetState");
  if (!ZES_FABRIC_PORT_GET_STATE_PTR && verbose)
    fprintf(stderr, "Missing symbol zesFabricPortGetState!\n");

  ZES_FABRIC_PORT_GET_THROUGHPUT_PTR =
      (zesFabricPortGetThroughput_t)(intptr_t)dlsym(handle, "zesFabricPortGetThroughput");
  if (!ZES_FABRIC_PORT_GET_THROUGHPUT_PTR && verbose)
    fprintf(stderr, "Missing symbol zesFabricPortGetThroughput!\n");

  ZES_DEVICE_ENUM_MEMORY_MODULES_PTR =
      (zesDeviceEnumMemoryModules_t)(intptr_t)dlsym(handle, "zesDeviceEnumMemoryModules");
  if (!ZES_DEVICE_ENUM_MEMORY_MODULES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesDeviceEnumMemoryModules!\n");

  ZES_MEMORY_GET_PROPERTIES_PTR =
      (zesMemoryGetProperties_t)(intptr_t)dlsym(handle, "zesMemoryGetProperties");
  if (!ZES_MEMORY_GET_PROPERTIES_PTR && verbose)
    fprintf(stderr, "Missing symbol zesMemoryGetProperties!\n");

  ZES_MEMORY_GET_STATE_PTR = (zesMemoryGetState_t)(intptr_t)dlsym(handle, "zesMemoryGetState");
  if (!ZES_MEMORY_GET_STATE_PTR && verbose)
    fprintf(stderr, "Missing symbol zesMemoryGetState!\n");

  ZES_MEMORY_GET_BANDWIDTH_PTR =
      (zesMemoryGetBandwidth_t)(intptr_t)dlsym(handle, "zesMemoryGetBandwidth");
  if (!ZES_MEMORY_GET_BANDWIDTH_PTR && verbose)
    fprintf(stderr, "Missing symbol zesMemoryGetBandwidth!\n");
}
volatile bool running = true;
thapi_sampling_handle_t _sampling_handle = NULL;
static int _sampling_freq_initialized = 0;
static int _sampling_fabricPorts_initialized = 0;
static int _sampling_memModules_initialized = 0;
static int _sampling_pwr_initialized = 0;
static int _sampling_engines_initialized = 0;
// Static handles to stay throughout the execution
static zes_driver_handle_t *_sampling_hDrivers = NULL;
static zes_device_handle_t **_sampling_hDevices = NULL;
static zes_freq_handle_t ***_sampling_hFrequencies = NULL;
static zes_pwr_handle_t ***_sampling_hPowers = NULL;
static zes_engine_handle_t ***_sampling_engineHandles = NULL;
static zes_fabric_port_handle_t ***_sampling_hFabricPort = NULL;
static zes_mem_handle_t ***_sampling_hMemModule = NULL;
static uint32_t _sampling_driverCount = 0;
static uint32_t *_sampling_deviceCount = NULL;
static uint32_t **_sampling_freqDomainCounts = NULL;
static uint32_t **_sampling_fabricPortCount = NULL;
static uint32_t **_sampling_memModuleCount = NULL;
static uint32_t **_sampling_powerDomainCounts = NULL;
static uint32_t **_sampling_engineCounts = NULL;

////////////////////////////////////////////
#define _ZE_ERROR_MSG(NAME,RES) do {\
  fprintf(stderr,"%s() failed at %d(%s): res=%x\n",(NAME),__LINE__,__FILE__,(RES));\
} while (0)
#define _ZE_ERROR_MSG_NOTERMINATE(NAME,RES) do {\
  fprintf(stderr,"%s() error at %d(%s): res=%x\n",(NAME),__LINE__,__FILE__,(RES));\
} while (0)
#define _ERROR_MSG(MSG) do {\
perror((MSG));fprintf(stderr, "errno=%d at %d(%s)\n", errno, __LINE__, __FILE__);\
} while (0)
#define _USAGE_MSG(MSG, ARGV0) do {\
  fprintf(stderr, "Usage: %s %s\n", (ARGV0), (MSG));\
} while (0)
#define _DL_ERROR_MSG() do {\
  fprintf(stderr, "dlopen error: %s at %d(%s)\n", dlerror(), __LINE__, __FILE__);\
} while(0)

static void intializeFrequency() {
  ze_result_t res;
  _sampling_hFrequencies =
      (zes_freq_handle_t ***)calloc(_sampling_driverCount, sizeof(zes_freq_handle_t **));
  _sampling_freqDomainCounts = (uint32_t **)calloc(_sampling_driverCount, sizeof(uint32_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    _sampling_freqDomainCounts[driverIdx] =
        (uint32_t *)calloc(_sampling_deviceCount[driverIdx], sizeof(uint32_t));
    _sampling_hFrequencies[driverIdx] =
        (zes_freq_handle_t **)calloc(_sampling_deviceCount[driverIdx], sizeof(zes_freq_handle_t *));
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      // Get frequency domains for each device
      res = ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                                  &_sampling_freqDomainCounts[driverIdx][deviceIdx],
                                                  NULL);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("1st ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR", res);
        _sampling_freqDomainCounts[driverIdx][deviceIdx] = 0;
        continue;
      }
      _sampling_hFrequencies[driverIdx][deviceIdx] = (zes_freq_handle_t *)calloc(
          _sampling_freqDomainCounts[driverIdx][deviceIdx], sizeof(zes_freq_handle_t));
      res = ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                                  &_sampling_freqDomainCounts[driverIdx][deviceIdx],
                                                  _sampling_hFrequencies[driverIdx][deviceIdx]);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("2nd ZES_DEVICE_ENUM_FREQUENCY_DOMAINS_PTR", res);
        _sampling_freqDomainCounts[driverIdx][deviceIdx] = 0;
        free(_sampling_hFrequencies[driverIdx][deviceIdx]);
      }
      for (uint32_t domainIdx = 0; domainIdx < _sampling_freqDomainCounts[driverIdx][deviceIdx];
           domainIdx++) {
        zes_freq_properties_t freqProps = {0};
        freqProps.stype = ZES_STRUCTURE_TYPE_FREQ_PROPERTIES;
        res = ZES_FREQUENCY_GET_PROPERTIES_PTR(
            _sampling_hFrequencies[driverIdx][deviceIdx][domainIdx], &freqProps);
        if (res != ZE_RESULT_SUCCESS) {
          _ZE_ERROR_MSG("ZES_FREQUENCY_GET_PROPERTIES_PTR", res);
          free(_sampling_hFrequencies[driverIdx][deviceIdx][domainIdx]);
        }
        do_tracepoint(lttng_ust_ze_sampling, freqProperties,
                      (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                      (zes_freq_handle_t)_sampling_hFrequencies[driverIdx][deviceIdx][domainIdx],
                      &freqProps);
      }
    }
  }
  _sampling_freq_initialized = 1;
}

static void intializePower() {
  ze_result_t res;
  _sampling_hPowers =
      (zes_pwr_handle_t ***)calloc(_sampling_driverCount, sizeof(zes_pwr_handle_t **));
  _sampling_powerDomainCounts = (uint32_t **)calloc(_sampling_driverCount, sizeof(uint32_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    _sampling_hPowers[driverIdx] =
        (zes_pwr_handle_t **)calloc(_sampling_deviceCount[driverIdx], sizeof(zes_pwr_handle_t *));
    _sampling_powerDomainCounts[driverIdx] =
        (uint32_t *)calloc(_sampling_deviceCount[driverIdx], sizeof(uint32_t));
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      // Get power domains for each device
      res = ZES_DEVICE_ENUM_POWER_DOMAINS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                              &_sampling_powerDomainCounts[driverIdx][deviceIdx],
                                              NULL);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("1st ZES_DEVICE_ENUM_POWER_DOMAINS_PTR", res);
        _sampling_powerDomainCounts[driverIdx][deviceIdx] = 0;
        continue;
      }
      _sampling_hPowers[driverIdx][deviceIdx] = (zes_pwr_handle_t *)calloc(
          _sampling_powerDomainCounts[driverIdx][deviceIdx], sizeof(zes_pwr_handle_t));
      res = ZES_DEVICE_ENUM_POWER_DOMAINS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                              &_sampling_powerDomainCounts[driverIdx][deviceIdx],
                                              _sampling_hPowers[driverIdx][deviceIdx]);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("2nd ZES_DEVICE_ENUM_POWER_DOMAINS_PTR", res);
        _sampling_powerDomainCounts[driverIdx][deviceIdx] = 0;
        free(_sampling_hPowers[driverIdx][deviceIdx]);
      }
      for (uint32_t domainIdx = 0; domainIdx < _sampling_powerDomainCounts[driverIdx][deviceIdx];
           domainIdx++) {
        zes_power_properties_t powerProperties = {0};
        powerProperties.stype = ZES_STRUCTURE_TYPE_POWER_PROPERTIES;
        res = ZES_POWER_GET_PROPERTIES_PTR(_sampling_hPowers[driverIdx][deviceIdx][domainIdx],
                                           &powerProperties);
        if (res != ZE_RESULT_SUCCESS) {
          _ZE_ERROR_MSG("ZES_POWER_GET_PROPERTIES_PTR", res);
          free(_sampling_hPowers[driverIdx][deviceIdx][domainIdx]);
        }
        do_tracepoint(lttng_ust_ze_sampling, powerProperties,
                      (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                      (zes_pwr_handle_t)_sampling_hPowers[driverIdx][deviceIdx][domainIdx],
                      &powerProperties);
      }
    }
  }
  _sampling_pwr_initialized = 1;
}

static void intializeEngines() {
  ze_result_t res;
  _sampling_engineHandles =
      (zes_engine_handle_t ***)calloc(_sampling_driverCount, sizeof(zes_engine_handle_t **));
  _sampling_engineCounts = (uint32_t **)calloc(_sampling_driverCount, sizeof(uint32_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    _sampling_engineHandles[driverIdx] = (zes_engine_handle_t **)calloc(
        _sampling_deviceCount[driverIdx], sizeof(zes_engine_handle_t *));
    _sampling_engineCounts[driverIdx] =
        (uint32_t *)calloc(_sampling_deviceCount[driverIdx], sizeof(uint32_t));
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      // Get engine counts for each device
      res = ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                              &_sampling_engineCounts[driverIdx][deviceIdx], NULL);
      if (res != ZE_RESULT_SUCCESS || _sampling_engineCounts[driverIdx][deviceIdx] == 0) {
        _ZE_ERROR_MSG("1st ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR", res);
        _sampling_engineCounts[driverIdx][deviceIdx] = 0;
        continue;
      }
      _sampling_engineHandles[driverIdx][deviceIdx] = (zes_engine_handle_t *)calloc(
          _sampling_engineCounts[driverIdx][deviceIdx], sizeof(zes_engine_handle_t));
      res = ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                              &_sampling_engineCounts[driverIdx][deviceIdx],
                                              _sampling_engineHandles[driverIdx][deviceIdx]);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("2nd ZES_DEVICE_ENUM_ENGINE_GROUPS_PTR", res);
        _sampling_engineCounts[driverIdx][deviceIdx] = 0;
        free(_sampling_engineHandles[driverIdx][deviceIdx]);
      }
      for (uint32_t engineIdx = 0; engineIdx < _sampling_engineCounts[driverIdx][deviceIdx];
           ++engineIdx) {
        zes_engine_properties_t engineProps = {0};
        engineProps.stype = ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES;
        res = ZES_ENGINE_GET_PROPERTIES_PTR(
            _sampling_engineHandles[driverIdx][deviceIdx][engineIdx], &engineProps);
        if (res != ZE_RESULT_SUCCESS) {
          _ZE_ERROR_MSG("ZES_ENGINE_GET_PROPERTIES_PTR", res);
        }
        do_tracepoint(lttng_ust_ze_sampling, engineProperties,
                      (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                      (zes_engine_handle_t)_sampling_engineHandles[driverIdx][deviceIdx][engineIdx],
                      &engineProps);
      }
    }
  }
  _sampling_engines_initialized = 1;
}

static void intializeFabricPorts() {
  ze_result_t res;
  _sampling_hFabricPort = (zes_fabric_port_handle_t ***)calloc(_sampling_driverCount,
                                                               sizeof(zes_fabric_port_handle_t **));
  _sampling_fabricPortCount = (uint32_t **)calloc(_sampling_driverCount, sizeof(uint32_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    _sampling_fabricPortCount[driverIdx] =
        (uint32_t *)calloc(_sampling_deviceCount[driverIdx], sizeof(uint32_t));
    _sampling_hFabricPort[driverIdx] = (zes_fabric_port_handle_t **)calloc(
        _sampling_deviceCount[driverIdx], sizeof(zes_fabric_port_handle_t *));
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      // Get fabric ports for each device
      res =
          ZES_DEVICE_ENUM_FABRIC_PORTS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                           &_sampling_fabricPortCount[driverIdx][deviceIdx], NULL);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("1st ZES_DEVICE_ENUM_FABRIC_PORTS_PTR", res);
        _sampling_fabricPortCount[driverIdx][deviceIdx] = 0;
        continue;
      }
      _sampling_hFabricPort[driverIdx][deviceIdx] = (zes_fabric_port_handle_t *)calloc(
          _sampling_fabricPortCount[driverIdx][deviceIdx], sizeof(zes_fabric_port_handle_t));
      res = ZES_DEVICE_ENUM_FABRIC_PORTS_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                             &_sampling_fabricPortCount[driverIdx][deviceIdx],
                                             _sampling_hFabricPort[driverIdx][deviceIdx]);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("2nd ZES_DEVICE_ENUM_FABRIC_PORTS_PTR", res);
        _sampling_fabricPortCount[driverIdx][deviceIdx] = 0;
        free(_sampling_hFabricPort[driverIdx][deviceIdx]);
      }
      for (uint32_t fabricPortIdx = 0;
           fabricPortIdx < _sampling_fabricPortCount[driverIdx][deviceIdx]; ++fabricPortIdx) {

        zes_fabric_port_properties_t fabricPortProps = {0};
        res = ZES_FABRIC_PORT_GET_PROPERTIES_PTR(
            _sampling_hFabricPort[driverIdx][deviceIdx][fabricPortIdx], &fabricPortProps);
        if (res != ZE_RESULT_SUCCESS) {
          _ZE_ERROR_MSG("ZES_FABRIC_PORT_GET_PROPERTIES_PTR", res);
        }
        // Dump fabricPortProperties once
        do_tracepoint(
            lttng_ust_ze_sampling, fabricPortProperties,
            (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
            (zes_fabric_port_handle_t)_sampling_hFabricPort[driverIdx][deviceIdx][fabricPortIdx],
            &fabricPortProps);
      }
    }
  }
  _sampling_fabricPorts_initialized = 1;
}

static void intializeMemModules() {
  ze_result_t res;
  _sampling_hMemModule =
      (zes_mem_handle_t ***)calloc(_sampling_driverCount, sizeof(zes_mem_handle_t **));
  _sampling_memModuleCount = (uint32_t **)calloc(_sampling_driverCount, sizeof(uint32_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    _sampling_memModuleCount[driverIdx] =
        (uint32_t *)calloc(_sampling_deviceCount[driverIdx], sizeof(uint32_t));
    _sampling_hMemModule[driverIdx] =
        (zes_mem_handle_t **)calloc(_sampling_deviceCount[driverIdx], sizeof(zes_mem_handle_t *));
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      // Get fabric ports for each device
      res =
          ZES_DEVICE_ENUM_MEMORY_MODULES_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                             &_sampling_memModuleCount[driverIdx][deviceIdx], NULL);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("1st ZES_DEVICE_ENUM_MEMORY_MODULES_PTR", res);
        _sampling_memModuleCount[driverIdx][deviceIdx] = 0;
        continue;
      }
      _sampling_hMemModule[driverIdx][deviceIdx] = (zes_mem_handle_t *)calloc(
          _sampling_memModuleCount[driverIdx][deviceIdx], sizeof(zes_mem_handle_t));
      res = ZES_DEVICE_ENUM_MEMORY_MODULES_PTR(_sampling_hDevices[driverIdx][deviceIdx],
                                               &_sampling_memModuleCount[driverIdx][deviceIdx],
                                               _sampling_hMemModule[driverIdx][deviceIdx]);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("2nd ZES_DEVICE_ENUM_MEMORY_MODULES_PTR", res);
        _sampling_memModuleCount[driverIdx][deviceIdx] = 0;
        free(_sampling_hMemModule[driverIdx][deviceIdx]);
      }
      for (uint32_t memModuleIdx = 0; memModuleIdx < _sampling_memModuleCount[driverIdx][deviceIdx];
           ++memModuleIdx) {
        zes_mem_properties_t memProps = {0};
        memProps.stype = ZES_STRUCTURE_TYPE_MEM_PROPERTIES;
        res = ZES_MEMORY_GET_PROPERTIES_PTR(
            _sampling_hMemModule[driverIdx][deviceIdx][memModuleIdx], &memProps);
        if (res != ZE_RESULT_SUCCESS) {
          _ZE_ERROR_MSG("ZES_MEMORY_GET_PROPERTIES_PTR", res);
        }
        // Dump fabricPortProperties once
        do_tracepoint(lttng_ust_ze_sampling, memoryProperties,
                      (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                      (zes_mem_handle_t)_sampling_hMemModule[driverIdx][deviceIdx][memModuleIdx],
                      &memProps);
      }
    }
  }
  _sampling_memModules_initialized = 1;
}

static int initializeHandles() {
  ze_result_t res;
  // find_ze_symbols(handle, NULL);
  res = ZES_INIT_PTR(0);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("ZES_INIT_PTR", res);
    return -1;
  }

  // Query driver
  _sampling_driverCount = 0;
  res = ZES_DRIVER_GET_PTR(&_sampling_driverCount, NULL);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("1st ZES_DRIVER_GET_PTR", res);
    return -1;
  }
  _sampling_hDrivers =
      (zes_driver_handle_t *)calloc(_sampling_driverCount, sizeof(zes_driver_handle_t));
  res = ZES_DRIVER_GET_PTR(&_sampling_driverCount, _sampling_hDrivers);
  if (res != ZE_RESULT_SUCCESS) {
    _ZE_ERROR_MSG("2nd ZES_DRIVER_GET_PTR", res);
    return -1;
  }
  _sampling_deviceCount = (uint32_t *)calloc(_sampling_driverCount, sizeof(uint32_t));
  _sampling_hDevices =
      (zes_device_handle_t **)calloc(_sampling_driverCount, sizeof(zes_device_handle_t *));
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    res =
        ZES_DEVICE_GET_PTR(_sampling_hDrivers[driverIdx], &_sampling_deviceCount[driverIdx], NULL);
    if (res != ZE_RESULT_SUCCESS || _sampling_deviceCount[driverIdx] == 0) {
      fprintf(stderr, "ERROR: No device found!\n");
      _ZE_ERROR_MSG("1st ZES_DEVICE_GET_PTR", res);
      return -1;
    }
    _sampling_hDevices[driverIdx] = (zes_device_handle_t *)calloc(_sampling_deviceCount[driverIdx],
                                                                  sizeof(zes_device_handle_t));
    res = ZES_DEVICE_GET_PTR(_sampling_hDrivers[driverIdx], &_sampling_deviceCount[driverIdx],
                             _sampling_hDevices[driverIdx]);
    if (res != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("2nd ZES_DEVICE_GET_PTR", res);
      free(_sampling_hDevices[driverIdx]);
      return -1;
    }
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {

      zes_device_properties_t deviceProps = {0};
      deviceProps.stype = ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES;
      deviceProps.pNext = NULL;
      res = ZES_DEVICE_GET_PROPERTIES_PTR(_sampling_hDevices[driverIdx][deviceIdx], &deviceProps);
      if (res != ZE_RESULT_SUCCESS) {
        _ZE_ERROR_MSG("ZES_DEVICE_GET_PROPERTIES_PTR", res);
      }
      do_tracepoint(lttng_ust_ze_sampling, deviceProperties,
                    (zes_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx], deviceIdx,
                    &deviceProps);
    }
  }
  intializeFrequency();
  intializePower();
  intializeEngines();
  intializeFabricPorts();
  intializeMemModules();
  return 0;
}

static void readFrequency_dump(uint32_t driverIdx, uint32_t deviceIdx) {
  if (!_sampling_freq_initialized)
    return;
  ze_result_t result;
  for (uint32_t domainIdx = 0; domainIdx < _sampling_freqDomainCounts[driverIdx][deviceIdx];
       domainIdx++) {
    zes_freq_state_t freqState = {0};
    result = ZES_FREQUENCY_GET_STATE_PTR(_sampling_hFrequencies[driverIdx][deviceIdx][domainIdx],
                                         &freqState);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_FREQUENCY_GET_STATE_PTR", result);
      continue;
    }
    do_tracepoint(lttng_ust_ze_sampling, gpu_frequency,
                  (zes_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                  (zes_freq_handle_t)_sampling_hFrequencies[driverIdx][deviceIdx][domainIdx],
                  domainIdx, &freqState);
  }
}

static void readFabricPorts_dump(uint32_t driverIdx, uint32_t deviceIdx) {
  if (!_sampling_fabricPorts_initialized)
    return;
  ze_result_t result;
  for (uint32_t portIdx = 0; portIdx < _sampling_fabricPortCount[driverIdx][deviceIdx]; portIdx++) {
    zes_fabric_port_state_t portState = {0};
    portState.pNext = NULL;
    portState.stype = ZES_STRUCTURE_TYPE_FABRIC_PORT_STATE;
    result = ZES_FABRIC_PORT_GET_STATE_PTR(_sampling_hFabricPort[driverIdx][deviceIdx][portIdx],
                                           &portState);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_FABRIC_PORT_GET_STATE_PTR", result);
      continue;
    }
    zes_fabric_port_throughput_t throughput = {0};
    result = ZES_FABRIC_PORT_GET_THROUGHPUT_PTR(
        _sampling_hFabricPort[driverIdx][deviceIdx][portIdx], &throughput);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_FABRIC_PORT_GET_THROUGHPUT_PTR", result);
      continue;
    }
    do_tracepoint(lttng_ust_ze_sampling, fabricPort,
                  (zes_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                  (zes_fabric_port_handle_t)_sampling_hFabricPort[driverIdx][deviceIdx][portIdx],
                  &portState, &throughput);
  }
}

static void readMemModules_dump(uint32_t driverIdx, uint32_t deviceIdx) {
  if (!_sampling_memModules_initialized)
    return;
  ze_result_t result;
  for (uint32_t memModuleIdx = 0; memModuleIdx < _sampling_memModuleCount[driverIdx][deviceIdx];
       ++memModuleIdx) {
    zes_mem_state_t memState = {0};
    memState.stype = ZES_STRUCTURE_TYPE_MEM_STATE;
    zes_mem_bandwidth_t memBandwidth = {0};
    result = ZES_MEMORY_GET_STATE_PTR(_sampling_hMemModule[driverIdx][deviceIdx][memModuleIdx],
                                      &memState);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_MEMORY_GET_STATE_PTR", result);
      continue;
    }
    result = ZES_MEMORY_GET_BANDWIDTH_PTR(_sampling_hMemModule[driverIdx][deviceIdx][memModuleIdx],
                                          &memBandwidth);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_MEMORY_GET_BANDWIDTH_PTR", result);
      continue;
    }
    do_tracepoint(lttng_ust_ze_sampling, memStats,
                  (zes_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                  (zes_mem_handle_t)_sampling_hMemModule[driverIdx][deviceIdx][memModuleIdx],
                  &memState, &memBandwidth);
  }
}

static void readEnergy_dump(uint32_t driverIdx, uint32_t deviceIdx) {
  if (!_sampling_pwr_initialized)
    return;
  ze_result_t result;
  for (uint32_t domainIdx = 0; domainIdx < _sampling_powerDomainCounts[driverIdx][deviceIdx];
       domainIdx++) {
    zes_power_energy_counter_t energyCounter = {0};
    result = ZES_POWER_GET_ENERGY_COUNTER_PTR(_sampling_hPowers[driverIdx][deviceIdx][domainIdx],
                                              &energyCounter);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_POWER_GET_ENERGY_COUNTER_PTR", result);
      continue;
    }
    do_tracepoint(lttng_ust_ze_sampling, gpu_energy,
                  (ze_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                  (zes_pwr_handle_t)_sampling_hPowers[driverIdx][deviceIdx][domainIdx], domainIdx,
                  &energyCounter);
  }
}

static void readEngines_dump(uint32_t driverIdx, uint32_t deviceIdx) {
  if (!_sampling_engines_initialized)
    return;
  ze_result_t result;
  for (uint32_t engineIdx = 0; engineIdx < _sampling_engineCounts[driverIdx][deviceIdx];
       ++engineIdx) {
    zes_engine_stats_t engineStats = {0};
    result = ZES_ENGINE_GET_ACTIVITY_PTR(_sampling_engineHandles[driverIdx][deviceIdx][engineIdx],
                                         &engineStats);
    if (result != ZE_RESULT_SUCCESS) {
      _ZE_ERROR_MSG("ZES_ENGINE_GET_ACTIVITY_PTR", result);
      continue;
    }
    do_tracepoint(lttng_ust_ze_sampling, engineStats,
                  (zes_device_handle_t)_sampling_hDevices[driverIdx][deviceIdx],
                  (zes_engine_handle_t)_sampling_engineHandles[driverIdx][deviceIdx][engineIdx],
                  &engineStats);
  }
}

static void thapi_sampling_energy() {
  for (uint32_t driverIdx = 0; driverIdx < _sampling_driverCount; driverIdx++) {
    for (uint32_t deviceIdx = 0; deviceIdx < _sampling_deviceCount[driverIdx]; deviceIdx++) {
      if (tracepoint_enabled(lttng_ust_ze_sampling, gpu_frequency)) {
        readFrequency_dump(driverIdx, deviceIdx);
      }
      if (tracepoint_enabled(lttng_ust_ze_sampling, gpu_energy)) {
        readEnergy_dump(driverIdx, deviceIdx);
      }
      if (tracepoint_enabled(lttng_ust_ze_sampling, engineStats)) {
        readEngines_dump(driverIdx, deviceIdx);
      }
      if (tracepoint_enabled(lttng_ust_ze_sampling, fabricPort)) {
        readFabricPorts_dump(driverIdx, deviceIdx);
      }
      if (tracepoint_enabled(lttng_ust_ze_sampling, memStats)) {
        readMemModules_dump(driverIdx, deviceIdx);
      }
    }
  }
}

void process_sampling() {
  struct timespec interval;
  interval.tv_sec = 0;
  interval.tv_nsec = 50000000; // 50ms interval
  thapi_sampling_energy();
  _sampling_handle = thapi_register_sampling(&thapi_sampling_energy, &interval);
}

void cleanup_sampling() {
  if (_sampling_handle) {
    thapi_unregister_sampling(_sampling_handle);
    _sampling_handle = NULL;
  }
}

void signal_handler(int signum) {
  if (signum == SIG_SAMPLING_FINISH) {
    cleanup_sampling();
    running = false;
  }
}

int main(int argc, char **argv) {
  
  int parent_pid = 0;
  int verbose = 0;
  void *handle = NULL;
  if (argc < 2) {
    _USAGE_MSG("<parent_pid>", argv[0]);
    return 1;
  }
  parent_pid = atoi(argv[1]);
  if (parent_pid <= 0) {
     _ERROR_MSG("Invalid or missing parent PID.");
     return 1;
  }
  
  thapi_sampling_init();// Initialize sampling

  // Load necessary libraries
  char *s = getenv("LTTNG_UST_ZE_LIBZE_LOADER");
  if (s) {
    handle = dlopen(s, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  } else {
    handle = dlopen("libze_loader.so", RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
  }

  if (!handle) {
    _DL_ERROR_MSG();
    return 1;
  }
  //Find zes symbols
  find_ze_symbols(handle, verbose);
  //Initialize device and telemetry handles
  initializeHandles();
  // Run the signal loop
  signal(SIG_SAMPLING_FINISH, signal_handler);

  if (kill(parent_pid, SIG_SAMPLING_READY) != 0) {
    _ERROR_MSG("Failed to send READY signal to parent");
  }
  // Process_sampling loop until SIG_SAMPLING_FINISH signal
  while (running) {
    process_sampling();
  }
  if (parent_pid != 0)
    kill(parent_pid, SIG_SAMPLING_READY);
  
  dlclose(handle);
  return 0;
}
