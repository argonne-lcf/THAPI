/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zex_driver.h
 *
 * @brief Intel 'One API' Level-Zero APIs
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/experimental/driver.yml
 * @endcond
 *
 */
#ifndef _ZEX_DRIVER_H
#define _ZEX_DRIVER_H
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
zexInit(
    ze_init_flag_t flags
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEX_DRIVER_H
