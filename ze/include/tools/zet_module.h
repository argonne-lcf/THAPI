/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_module.h
 *
 * @brief Intel 'One API' Level-Zero Tool APIs for Module
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/module.yml
 * @endcond
 *
 */
#ifndef _ZET_MODULE_H
#define _ZET_MODULE_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _zet_module_debug_info_format_t
{
    ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF,

} zet_module_debug_info_format_t;

ze_result_t __zecall
zetModuleGetDebugInfo(
    zet_module_handle_t hModule,
    zet_module_debug_info_format_t format,
    size_t* pSize,
    uint8_t* pDebugInfo
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_MODULE_H
