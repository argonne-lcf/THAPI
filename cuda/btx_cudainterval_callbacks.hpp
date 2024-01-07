#pragma once

#include <iostream>

#include "cuda.h.include"

#include "xprof_utils.hpp"

using hp_event_t = std::tuple<hostname_t, process_id_t, CUevent, CUevent>;
using hp_kernel_t = std::tuple<hostname_t, process_id_t, CUfunction>;
using hp_stream_t = std::tuple<hostname_t, process_id_t, CUstream>;
using hpt_context_t = std::tuple<hostname_t, process_id_t, thread_id_t, CUcontext>;
using hp_context_t = std::tuple<hostname_t, process_id_t, CUcontext>;
using context_stack_t = std::stack<CUcontext>;

inline bool cuResultIsError(CUresult& cuResult) {
  return (cuResult != CUDA_SUCCESS) && (cuResult != CUDA_ERROR_NOT_READY);
}

#define THAPI_FATAL(msg) __thapi_fatal(msg, __FILE__, __LINE__)
#define THAPI_FATAL_HPT(hpt, msg) __thapi_fatal_hpt(hpt, msg, __FILE__, __LINE__)

inline void __thapi_fatal(const char *msg, const char *fname, int line) {
  std::cerr << "ERROR at " << fname << " " << line << ": " << msg << std::endl;
  abort();
}

inline void __thapi_fatal_hpt(hpt_t hpt, const char *msg, const char *fname, int line) {
  std::cerr << "ERROR at " << fname << " " << line << ": " << msg
            << " {" << std::get<0>(hpt) << ", " << std::get<1>(hpt)
            << ", " << std::get<2>(hpt) << "}" << std::endl;
  abort();
}
