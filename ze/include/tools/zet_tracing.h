/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zet_tracing.h
 *
 * @brief Intel 'One API' Level-Zero Tool APIs for API Tracing
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/tools/tracing.yml
 * @endcond
 *
 */
#ifndef _ZET_TRACING_H
#define _ZET_TRACING_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZET_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif
#include "ze_callbacks.h"
#include "zex_callbacks.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef ze_callbacks_t zet_core_callbacks_t;

typedef zex_callbacks_t zet_experimental_callbacks_t;

typedef enum _zet_tracer_desc_version_t
{
    ZET_TRACER_DESC_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 0 ),

} zet_tracer_desc_version_t;

typedef struct _zet_tracer_desc_t
{
    zet_tracer_desc_version_t version;
    void* pUserData;

} zet_tracer_desc_t;

ze_result_t __zecall
zetTracerCreate(
    zet_device_handle_t hDevice,
    const zet_tracer_desc_t* desc,
    zet_tracer_handle_t* phTracer
    );

ze_result_t __zecall
zetTracerDestroy(
    zet_tracer_handle_t hTracer
    );

ze_result_t __zecall
zetTracerSetPrologues(
    zet_tracer_handle_t hTracer,
    zet_core_callbacks_t* pCoreCbs,
    zet_experimental_callbacks_t* pExperimentalCbs
    );

ze_result_t __zecall
zetTracerSetEpilogues(
    zet_tracer_handle_t hTracer,
    zet_core_callbacks_t* pCoreCbs,
    zet_experimental_callbacks_t* pExperimentalCbs
    );

ze_result_t __zecall
zetTracerSetEnabled(
    zet_tracer_handle_t hTracer,
    ze_bool_t enable
    );

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZET_TRACING_H
