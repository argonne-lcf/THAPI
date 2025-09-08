/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zel_tracing_ddi.h
 *
 * This file has been manually generated.
 * There is no "spec" for this loader layer "tracer" API.
 */

#ifndef _ZEL_TRACING_DDI_VER_H
#define _ZEL_TRACING_DDI_VER_H
#if defined(__cplusplus)
#pragma once
#endif
#include "layers/zel_tracing_ddi.h"

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// [1.0]
/// @brief Table of Tracer functions pointers
typedef struct _zel_tracer_dditable_t_1_0
{
    zel_pfnTracerCreate_t                                    pfnCreate;
    zel_pfnTracerDestroy_t                                   pfnDestroy;
    zel_pfnTracerSetPrologues_t                              pfnSetPrologues;
    zel_pfnTracerSetEpilogues_t                              pfnSetEpilogues;
    zel_pfnTracerSetEnabled_t                                pfnSetEnabled;
} zel_tracer_dditable_t_1_0;


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZEL_TRACING_DDI_VER_H
