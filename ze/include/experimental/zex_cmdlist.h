/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_cmdlist.h
 *
 * @brief Intel 'One API' Level-Zero Experimental APIs for Command List
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental/cmdlist.yml
 * @endcond
 *
 */
#ifndef _ZEX_CMDLIST_H
#define _ZEX_CMDLIST_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZEX_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ze_result_t __zecall
zexCommandListReserveSpace(
    zex_command_list_handle_t hCommandList,
    size_t size,
    void** ptr
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_CMDLIST_H
