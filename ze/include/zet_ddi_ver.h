/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_ddi.h
 * @version v1.9-r1.9.3
 *
 */
#ifndef _ZET_DDI_VER_H
#define _ZET_DDI_VER_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zet_ddi.h"

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of MetricProgrammableExp functions pointers
typedef struct _zet_metric_programmable_exp_dditable_t_1_9
{
    zet_pfnMetricProgrammableGetExp_t                           pfnGetExp;
    zet_pfnMetricProgrammableGetPropertiesExp_t                 pfnGetPropertiesExp;
    zet_pfnMetricProgrammableGetParamInfoExp_t                  pfnGetParamInfoExp;
    zet_pfnMetricProgrammableGetParamValueInfoExp_t             pfnGetParamValueInfoExp;
} zet_metric_programmable_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Device functions pointers
typedef struct _zet_device_dditable_t_1_0
{
    zet_pfnDeviceGetDebugProperties_t                           pfnGetDebugProperties;
} zet_device_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Context functions pointers
typedef struct _zet_context_dditable_t_1_0
{
    zet_pfnContextActivateMetricGroups_t                        pfnActivateMetricGroups;
} zet_context_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of CommandList functions pointers
typedef struct _zet_command_list_dditable_t_1_0
{
    zet_pfnCommandListAppendMetricStreamerMarker_t              pfnAppendMetricStreamerMarker;
    zet_pfnCommandListAppendMetricQueryBegin_t                  pfnAppendMetricQueryBegin;
    zet_pfnCommandListAppendMetricQueryEnd_t                    pfnAppendMetricQueryEnd;
    zet_pfnCommandListAppendMetricMemoryBarrier_t               pfnAppendMetricMemoryBarrier;
} zet_command_list_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Module functions pointers
typedef struct _zet_module_dditable_t_1_0
{
    zet_pfnModuleGetDebugInfo_t                                 pfnGetDebugInfo;
} zet_module_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Kernel functions pointers
typedef struct _zet_kernel_dditable_t_1_0
{
    zet_pfnKernelGetProfileInfo_t                               pfnGetProfileInfo;
} zet_kernel_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Metric functions pointers
typedef struct _zet_metric_dditable_t_1_0
{
    zet_pfnMetricGet_t                                          pfnGet;
    zet_pfnMetricGetProperties_t                                pfnGetProperties;
} zet_metric_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.9]
/// @brief Table of MetricExp functions pointers
typedef struct _zet_metric_exp_dditable_t_1_9
{
    zet_pfnMetricCreateFromProgrammableExp_t                    pfnCreateFromProgrammableExp;
    zet_pfnMetricDestroyExp_t                                   pfnDestroyExp;
} zet_metric_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of MetricGroup functions pointers
typedef struct _zet_metric_group_dditable_t_1_0
{
    zet_pfnMetricGroupGet_t                                     pfnGet;
    zet_pfnMetricGroupGetProperties_t                           pfnGetProperties;
    zet_pfnMetricGroupCalculateMetricValues_t                   pfnCalculateMetricValues;
} zet_metric_group_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.2, 1.5, 1.6, 1.9]
/// @brief Table of MetricGroupExp functions pointers
typedef struct _zet_metric_group_exp_dditable_t_1_2
{
    zet_pfnMetricGroupCalculateMultipleMetricValuesExp_t        pfnCalculateMultipleMetricValuesExp;
} zet_metric_group_exp_dditable_t_1_2;

typedef struct _zet_metric_group_exp_dditable_t_1_5
{
    zet_pfnMetricGroupCalculateMultipleMetricValuesExp_t        pfnCalculateMultipleMetricValuesExp;
    zet_pfnMetricGroupGetGlobalTimestampsExp_t                  pfnGetGlobalTimestampsExp;
} zet_metric_group_exp_dditable_t_1_5;

typedef struct _zet_metric_group_exp_dditable_t_1_6
{
    zet_pfnMetricGroupCalculateMultipleMetricValuesExp_t        pfnCalculateMultipleMetricValuesExp;
    zet_pfnMetricGroupGetGlobalTimestampsExp_t                  pfnGetGlobalTimestampsExp;
    zet_pfnMetricGroupGetExportDataExp_t                        pfnGetExportDataExp;
    zet_pfnMetricGroupCalculateMetricExportDataExp_t            pfnCalculateMetricExportDataExp;
} zet_metric_group_exp_dditable_t_1_6;

typedef struct _zet_metric_group_exp_dditable_t_1_9
{
    zet_pfnMetricGroupCalculateMultipleMetricValuesExp_t        pfnCalculateMultipleMetricValuesExp;
    zet_pfnMetricGroupGetGlobalTimestampsExp_t                  pfnGetGlobalTimestampsExp;
    zet_pfnMetricGroupGetExportDataExp_t                        pfnGetExportDataExp;
    zet_pfnMetricGroupCalculateMetricExportDataExp_t            pfnCalculateMetricExportDataExp;
    zet_pfnMetricGroupCreateExp_t                               pfnCreateExp;
    zet_pfnMetricGroupAddMetricExp_t                            pfnAddMetricExp;
    zet_pfnMetricGroupRemoveMetricExp_t                         pfnRemoveMetricExp;
    zet_pfnMetricGroupCloseExp_t                                pfnCloseExp;
    zet_pfnMetricGroupDestroyExp_t                              pfnDestroyExp;
} zet_metric_group_exp_dditable_t_1_9;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of MetricStreamer functions pointers
typedef struct _zet_metric_streamer_dditable_t_1_0
{
    zet_pfnMetricStreamerOpen_t                                 pfnOpen;
    zet_pfnMetricStreamerClose_t                                pfnClose;
    zet_pfnMetricStreamerReadData_t                             pfnReadData;
} zet_metric_streamer_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of MetricQueryPool functions pointers
typedef struct _zet_metric_query_pool_dditable_t_1_0
{
    zet_pfnMetricQueryPoolCreate_t                              pfnCreate;
    zet_pfnMetricQueryPoolDestroy_t                             pfnDestroy;
} zet_metric_query_pool_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of MetricQuery functions pointers
typedef struct _zet_metric_query_dditable_t_1_0
{
    zet_pfnMetricQueryCreate_t                                  pfnCreate;
    zet_pfnMetricQueryDestroy_t                                 pfnDestroy;
    zet_pfnMetricQueryReset_t                                   pfnReset;
    zet_pfnMetricQueryGetData_t                                 pfnGetData;
} zet_metric_query_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of TracerExp functions pointers
typedef struct _zet_tracer_exp_dditable_t_1_0
{
    zet_pfnTracerExpCreate_t                                    pfnCreate;
    zet_pfnTracerExpDestroy_t                                   pfnDestroy;
    zet_pfnTracerExpSetPrologues_t                              pfnSetPrologues;
    zet_pfnTracerExpSetEpilogues_t                              pfnSetEpilogues;
    zet_pfnTracerExpSetEnabled_t                                pfnSetEnabled;
} zet_tracer_exp_dditable_t_1_0;

///////////////////////////////////////////////////////////////////////////////
/// [1.0, 1.5]
/// @brief Table of Debug functions pointers
typedef struct _zet_debug_dditable_t_1_0
{
    zet_pfnDebugAttach_t                                        pfnAttach;
    zet_pfnDebugDetach_t                                        pfnDetach;
    zet_pfnDebugReadEvent_t                                     pfnReadEvent;
    zet_pfnDebugAcknowledgeEvent_t                              pfnAcknowledgeEvent;
    zet_pfnDebugInterrupt_t                                     pfnInterrupt;
    zet_pfnDebugResume_t                                        pfnResume;
    zet_pfnDebugReadMemory_t                                    pfnReadMemory;
    zet_pfnDebugWriteMemory_t                                   pfnWriteMemory;
    zet_pfnDebugGetRegisterSetProperties_t                      pfnGetRegisterSetProperties;
    zet_pfnDebugReadRegisters_t                                 pfnReadRegisters;
    zet_pfnDebugWriteRegisters_t                                pfnWriteRegisters;
} zet_debug_dditable_t_1_0;

typedef struct _zet_debug_dditable_t_1_5
{
    zet_pfnDebugAttach_t                                        pfnAttach;
    zet_pfnDebugDetach_t                                        pfnDetach;
    zet_pfnDebugReadEvent_t                                     pfnReadEvent;
    zet_pfnDebugAcknowledgeEvent_t                              pfnAcknowledgeEvent;
    zet_pfnDebugInterrupt_t                                     pfnInterrupt;
    zet_pfnDebugResume_t                                        pfnResume;
    zet_pfnDebugReadMemory_t                                    pfnReadMemory;
    zet_pfnDebugWriteMemory_t                                   pfnWriteMemory;
    zet_pfnDebugGetRegisterSetProperties_t                      pfnGetRegisterSetProperties;
    zet_pfnDebugReadRegisters_t                                 pfnReadRegisters;
    zet_pfnDebugWriteRegisters_t                                pfnWriteRegisters;
    zet_pfnDebugGetThreadRegisterSetProperties_t                pfnGetThreadRegisterSetProperties;
} zet_debug_dditable_t_1_5;


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_DDI_VER_H
