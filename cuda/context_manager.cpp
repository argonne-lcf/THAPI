#include "context_manager.hpp"

#include <cassert>


CUdevice CUDAContextManager::get_device(hpt_t hpt) {
  CUcontext ctx = get_top_context(hpt);
  hp_context_t hp_context = {std::get<0>(hpt), std::get<1>(hpt), ctx};
  return THAPI_AT(hp_ctx_to_device_, hp_context);
}

CUcontext CUDAContextManager::get_top_context(hpt_t hpt) {
  // std::cerr << "get top ctx for thread " << std::get<2>(hpt) << std::endl;
  auto& hpt_stack = THAPI_AT(hpt_ctx_stack_, hpt);
  // Note: the primary context is not used automatically, it must be pushed or set
  // current first and be on the stack
  assert(!hpt_stack.empty());
  return hpt_stack.top();
}

CUdevice CUDAContextManager::get_stream_device(hpt_t hpt, CUstream stream) {
  if (stream == 0) {
    // using the default stream in the top context on the stack
    return get_device(hpt);
  }
  hp_stream_t hp_stream_key = {std::get<0>(hpt), std::get<1>(hpt), stream};
  auto ctx = THAPI_AT(hp_stream_to_ctx_, hp_stream_key);
  hp_context_t hp_context = {std::get<0>(hpt), std::get<1>(hpt), ctx};
  return THAPI_AT(hp_ctx_to_device_, hp_context);
}

// cuPrimaryCtxRetain_v2_(entry|exit)
void CUDAContextManager::primary_ctx_retain_entry(hpt_t hpt, CUdevice dev) {
  hpt_entry_dev_[hpt] = dev;
}

void CUDAContextManager::primary_ctx_retain_exit(hpt_t hpt, CUresult cuResult,
                                                 CUcontext ctx) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_dev = THAPI_AT(hpt_entry_dev_, hpt);
  hp_device_primary_ctx_[{std::get<0>(hpt), std::get<1>(hpt), entry_dev}] = ctx;
  hp_ctx_to_device_[{std::get<0>(hpt), std::get<1>(hpt), ctx}] = entry_dev;
}


// cuPrimaryCtxRelease_v2_(entry|exit)
void CUDAContextManager::primary_ctx_release_entry(hpt_t hpt, CUdevice dev) {
  hpt_entry_dev_[hpt] = dev;
}

void CUDAContextManager::primary_ctx_release_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  CUdevice entry_dev = THAPI_AT(hpt_entry_dev_, hpt);
  hp_device_t hp_dev_key = {std::get<0>(hpt), std::get<1>(hpt), entry_dev};
  auto ctx = THAPI_AT(hp_device_primary_ctx_, hp_dev_key);
  hp_device_primary_ctx_.erase(hp_dev_key);
  hp_ctx_to_device_.erase({std::get<0>(hpt), std::get<1>(hpt), ctx});
}

// cuPrimaryCtxReset_v2_(entry|exit)
void CUDAContextManager::primary_ctx_reset_entry(hpt_t hpt, CUdevice dev) {
  primary_ctx_release_entry(hpt, dev);
}

void CUDAContextManager::primary_ctx_reset_exit(hpt_t hpt, CUresult cuResult) {
  primary_ctx_release_exit(hpt, cuResult);
}


// cuCtxCreate_v(2|3)_(entry|exit)
void CUDAContextManager::ctx_create_entry(hpt_t hpt, CUdevice dev) {
  hpt_entry_dev_[hpt] = dev;
}

void CUDAContextManager::ctx_create_exit(hpt_t hpt, CUresult cuResult,
                                             CUcontext ctx) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_dev = THAPI_AT(hpt_entry_dev_, hpt);
  hp_context_t hp_ctx_key = {std::get<0>(hpt), std::get<1>(hpt), ctx};
  hp_ctx_to_device_[hp_ctx_key] = entry_dev;
  hpt_ctx_stack_[hpt].push(ctx);
}


// cuCtxDestroy_(entry|exit)
void CUDAContextManager::ctx_destroy_entry(hpt_t hpt, CUcontext ctx) {
  hpt_entry_ctx_[hpt] = ctx;
}

void CUDAContextManager::ctx_destroy_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  CUcontext entry_ctx;
  try {
    entry_ctx = THAPI_AT(hpt_entry_ctx_, hpt);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry device for thread in ctx destroy exit callback");
  }

  // std::cerr << "destroy ctx " << std::get<2>(hpt) << " " << entry_ctx << std::endl;
  // context is no longer usuable after destroy, trying to use it should trigger
  // an error
  hp_ctx_to_device_.erase({std::get<0>(hpt), std::get<1>(hpt), entry_ctx});

  // pop thread context stack only if it's current on the calling thread
  // See https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__CTX.html
  try {
    auto& stack = THAPI_AT(hpt_ctx_stack_, hpt);
    if (!stack.empty() && stack.top() == entry_ctx) {
      stack.pop();
    }
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no ctx stack for thread in ctx detroy exit callback");
  }
}


// cuCtxSetCurrent_(entry|exit)
void CUDAContextManager::ctx_set_current_entry(hpt_t hpt, CUcontext ctx) {
  // std::cerr << "set current entry thread " << std::get<2>(hpt) << std::endl;
  hpt_entry_ctx_[hpt] = ctx;
}

void CUDAContextManager::ctx_set_current_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_ctx = THAPI_AT(hpt_entry_ctx_, hpt);
  // Note: don't use THAPI_AT, we need operator[] semantics of
  // creating if the stack does not exists yet.
  auto& stack = hpt_ctx_stack_[hpt];
  if (!stack.empty()) {
    stack.pop();
  }
  stack.push(entry_ctx);
}

// cuCtxSetCurrent_(entry|exit)
void CUDAContextManager::ctx_push_current_entry(hpt_t hpt, CUcontext ctx) {
  hpt_entry_ctx_[hpt] = ctx;
}

void CUDAContextManager::ctx_push_current_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  // Note: do use THAPI_AT, stack should already exist for valid program
  auto entry_ctx = THAPI_AT(hpt_entry_ctx_, hpt);
  hpt_ctx_stack_[hpt].push(entry_ctx);
}


// cuCtxPopCurrent_(entry|exit)
void CUDAContextManager::ctx_pop_current_entry(hpt_t hpt) {
  // noop
}

void CUDAContextManager::ctx_pop_current_exit(hpt_t hpt, CUresult cuResult,
                                              CUcontext cuContext) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  hpt_ctx_stack_[hpt].pop();
}

// cuStreamCreate_exit
// Note: entry not needed
void CUDAContextManager::stream_create_exit(hpt_t hpt, CUresult cuResult,
                                            CUstream cuStream) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto ctx = get_top_context(hpt);
  hp_stream_t hp_stream_key = {std::get<0>(hpt), std::get<1>(hpt), cuStream}; 
  hp_stream_to_ctx_[hp_stream_key] = ctx;
}
