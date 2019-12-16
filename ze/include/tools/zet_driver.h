/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_driver.h
 *
 * @brief Intel 'One API' Level-Zero APIs
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/driver.yml
 * @endcond
 *
 */
#ifndef _ZET_DRIVER_H
#define _ZET_DRIVER_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ze_result_t __zecall
zetInit(
    ze_init_flag_t flags
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_DRIVER_H
