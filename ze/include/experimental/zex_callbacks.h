/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_callbacks.h
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental
 * @endcond
 *
 */
#ifndef _ZEX_CALLBACKS_H
#define _ZEX_CALLBACKS_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zex_api.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _zex_init_params_t
{
    ze_init_flag_t* pflags;
} zex_init_params_t;

typedef void (__zecall *zex_pfnInitCb_t)(
    zex_init_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _zex_global_callbacks_t
{
    zex_pfnInitCb_t                                                 pfnInitCb;
} zex_global_callbacks_t;

typedef struct _zex_command_list_reserve_space_params_t
{
    zex_command_list_handle_t* phCommandList;
    size_t* psize;
    void*** pptr;
} zex_command_list_reserve_space_params_t;

typedef void (__zecall *zex_pfnCommandListReserveSpaceCb_t)(
    zex_command_list_reserve_space_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _zex_command_list_callbacks_t
{
    zex_pfnCommandListReserveSpaceCb_t                              pfnReserveSpaceCb;
} zex_command_list_callbacks_t;

typedef struct _zex_command_graph_create_params_t
{
    ze_device_handle_t* phDevice;
    const zex_command_graph_desc_t** pdesc;
    zex_command_graph_handle_t** pphCommandGraph;
} zex_command_graph_create_params_t;

typedef void (__zecall *zex_pfnCommandGraphCreateCb_t)(
    zex_command_graph_create_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _zex_command_graph_destroy_params_t
{
    zex_command_graph_handle_t* phCommandGraph;
} zex_command_graph_destroy_params_t;

typedef void (__zecall *zex_pfnCommandGraphDestroyCb_t)(
    zex_command_graph_destroy_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _zex_command_graph_close_params_t
{
    zex_command_graph_handle_t* phCommandGraph;
} zex_command_graph_close_params_t;

typedef void (__zecall *zex_pfnCommandGraphCloseCb_t)(
    zex_command_graph_close_params_t* params,
    ze_result_t result,
    void* pTracerUserData,
    void** ppTracerInstanceUserData
    );

typedef struct _zex_command_graph_callbacks_t
{
    zex_pfnCommandGraphCreateCb_t                                   pfnCreateCb;
    zex_pfnCommandGraphDestroyCb_t                                  pfnDestroyCb;
    zex_pfnCommandGraphCloseCb_t                                    pfnCloseCb;
} zex_command_graph_callbacks_t;

typedef struct _zex_callbacks_t
{
    zex_global_callbacks_t              Global;
    zex_command_list_callbacks_t        CommandList;
    zex_command_graph_callbacks_t       CommandGraph;
} zex_callbacks_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_CALLBACKS_H
