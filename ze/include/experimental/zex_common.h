/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_common.h
 *
 * @brief Intel 'One API' Level-Zero Experimental API common types
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental/common.yml
 * @endcond
 *
 */
#ifndef _ZEX_COMMON_H
#define _ZEX_COMMON_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZEX_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef ze_command_list_handle_t zex_command_list_handle_t;

typedef struct _zex_command_graph_handle_t *zex_command_graph_handle_t;

//typedef struct _zex_command_graph_desc_t zex_command_graph_desc_t;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_COMMON_H
