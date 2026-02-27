/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file zer_ddi.h
 * @version v1.15-r1.13.73
 *
 */
#ifndef _ZER_DDI_VER_H
#define _ZER_DDI_VER_H
#if defined(__cplusplus)
#pragma once
#endif
#include "zer_ddi.h"

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// [1.14]
/// @brief Table of Global functions pointers
typedef struct _zer_global_dditable_t_1_14
{
    zer_pfnGetLastErrorDescription_t                            pfnGetLastErrorDescription;
    zer_pfnTranslateDeviceHandleToIdentifier_t                  pfnTranslateDeviceHandleToIdentifier;
    zer_pfnTranslateIdentifierToDeviceHandle_t                  pfnTranslateIdentifierToDeviceHandle;
    zer_pfnGetDefaultContext_t                                  pfnGetDefaultContext;
} zer_global_dditable_t_1_14;


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZER_DDI_VER_H
