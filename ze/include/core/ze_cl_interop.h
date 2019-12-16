/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * @file ze_cl_interop.h
 *
 * @brief Intel 'One API' Level-Zero APIs for OpenCL Interoperability
 *
 * @cond DEV
 * DO NOT EDIT: generated from /scripts/core/cl_interop.yml
 * @endcond
 *
 */
#ifndef _ZE_CL_INTEROP_H
#define _ZE_CL_INTEROP_H
#if defined(__cplusplus)
#pragma once
#endif
#if !defined(_ZE_API_H)
#pragma message("warning: this file is not intended to be included directly")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if ZE_ENABLE_OCL_INTEROP
ze_result_t __zecall
zeDeviceRegisterCLMemory(
    ze_device_handle_t hDevice,
    cl_context context,
    cl_mem mem,
    void** ptr
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
ze_result_t __zecall
zeDeviceRegisterCLProgram(
    ze_device_handle_t hDevice,
    cl_context context,
    cl_program program,
    ze_module_handle_t* phModule
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if ZE_ENABLE_OCL_INTEROP
ze_result_t __zecall
zeDeviceRegisterCLCommandQueue(
    ze_device_handle_t hDevice,
    cl_context context,
    cl_command_queue command_queue,
    ze_command_queue_handle_t* phCommandQueue
    );
#endif // ZE_ENABLE_OCL_INTEROP

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _ZE_CL_INTEROP_H
