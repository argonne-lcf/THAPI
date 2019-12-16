/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_cmdgraph.h
 *
 * @brief Intel 'One API' Level-Zero Experimental APIs for CommandGraph
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental/cmdgraph.yml
 * @endcond
 *
 */
#ifndef _ZEX_CMDGRAPH_H
#define _ZEX_CMDGRAPH_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZEX_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _zex_command_graph_desc_version_t
{
    ZEX_COMMAND_GRAPH_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zex_command_graph_desc_version_t;

typedef enum _zex_command_graph_flag_t
{
    ZEX_COMMAND_GRAPH_FLAG_NONE = 0,

} zex_command_graph_flag_t;

typedef struct _zex_command_graph_desc_t
{
    zex_command_graph_desc_version_t version;
    zex_command_graph_flag_t flags;

} zex_command_graph_desc_t;

ze_result_t __zecall
zexCommandGraphCreate(
    ze_device_handle_t hDevice,
    const zex_command_graph_desc_t* desc,
    zex_command_graph_handle_t* phCommandGraph
    );

ze_result_t __zecall
zexCommandGraphDestroy(
    zex_command_graph_handle_t hCommandGraph
    );

ze_result_t __zecall
zexCommandGraphClose(
    zex_command_graph_handle_t hCommandGraph
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_CMDGRAPH_H
