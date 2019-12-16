/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_ddi.h
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental
 * @endcond
 *
 */
#ifndef _ZEX_DDI_H
#define _ZEX_DDI_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zex_api.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef ze_result_t (__zecall *zex_pfnInit_t)(
    ze_init_flag_t
    );

typedef struct _zex_global_dditable_t
{
    zex_pfnInit_t                                               pfnInit;
} zex_global_dditable_t;

__zedllexport ze_result_t __zecall
zexGetGlobalProcAddrTable(
    ze_api_version_t version,
    zex_global_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zex_pfnGetGlobalProcAddrTable_t)(
    ze_api_version_t,
    zex_global_dditable_t*
    );

typedef ze_result_t (__zecall *zex_pfnCommandListReserveSpace_t)(
    zex_command_list_handle_t,
    size_t,
    void**
    );

typedef struct _zex_command_list_dditable_t
{
    zex_pfnCommandListReserveSpace_t                            pfnReserveSpace;
} zex_command_list_dditable_t;

__zedllexport ze_result_t __zecall
zexGetCommandListProcAddrTable(
    ze_api_version_t version,
    zex_command_list_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zex_pfnGetCommandListProcAddrTable_t)(
    ze_api_version_t,
    zex_command_list_dditable_t*
    );

typedef ze_result_t (__zecall *zex_pfnCommandGraphCreate_t)(
    ze_device_handle_t,
    const zex_command_graph_desc_t*,
    zex_command_graph_handle_t*
    );

typedef ze_result_t (__zecall *zex_pfnCommandGraphDestroy_t)(
    zex_command_graph_handle_t
    );

typedef ze_result_t (__zecall *zex_pfnCommandGraphClose_t)(
    zex_command_graph_handle_t
    );

typedef struct _zex_command_graph_dditable_t
{
    zex_pfnCommandGraphCreate_t                                 pfnCreate;
    zex_pfnCommandGraphDestroy_t                                pfnDestroy;
    zex_pfnCommandGraphClose_t                                  pfnClose;
} zex_command_graph_dditable_t;

__zedllexport ze_result_t __zecall
zexGetCommandGraphProcAddrTable(
    ze_api_version_t version,
    zex_command_graph_dditable_t* pDdiTable
    );

typedef ze_result_t (__zecall *zex_pfnGetCommandGraphProcAddrTable_t)(
    ze_api_version_t,
    zex_command_graph_dditable_t*
    );

typedef struct _zex_dditable_t
{
    zex_global_dditable_t               Global;
    zex_command_list_dditable_t         CommandList;
    zex_command_graph_dditable_t        CommandGraph;
} zex_dditable_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_DDI_H
