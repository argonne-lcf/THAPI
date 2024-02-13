#include "context_manager.hpp"

#include <cassert>

CUdevice CUDAContextManager::get_device(hpt_t hpt) {
  CUcontext ctx = get_top_context(hpt);
  hp_context_t hp_context{std::get<0>(hpt), std::get<1>(hpt), ctx};
  return thapi_at(hp_ctx_to_device_, hp_context);
}

CUcontext CUDAContextManager::get_top_context(hpt_t hpt) {
  auto &hpt_stack = thapi_at(hpt_ctx_stack_, hpt);
  // Note: the primary context is not used automatically, it must be pushed or
  // set current first and be on the stack
  assert(!hpt_stack.empty());
  return hpt_stack.top();
}

CUdevice CUDAContextManager::get_stream_device(hpt_t hpt, CUstream stream) {
  if (stream == 0) {
    // using the default stream in the top context on the stack
    return get_device(hpt);
  }
  hp_stream_t hp_stream_key{std::get<0>(hpt), std::get<1>(hpt), stream};
  auto ctx = thapi_at(hp_stream_to_ctx_, hp_stream_key);
  hp_context_t hp_context{std::get<0>(hpt), std::get<1>(hpt), ctx};
  return thapi_at(hp_ctx_to_device_, hp_context);
}

// cuPrimaryCtxRetain_v2_(entry|exit)
void CUDAContextManager::primary_ctx_retain_entry(hpt_t hpt, CUdevice dev) {
  entry_state_.set_data<CUdevice>(hpt, dev);
}

void CUDAContextManager::primary_ctx_retain_exit(hpt_t hpt, CUresult cuResult,
                                                 CUcontext ctx) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_dev = entry_state_.get_data<CUdevice>(hpt);
  hp_ctx_to_device_[{std::get<0>(hpt), std::get<1>(hpt), ctx}] = entry_dev;
}

// cuCtxCreate_v(2|3)_(entry|exit)
void CUDAContextManager::ctx_create_entry(hpt_t hpt, CUdevice dev) {
  entry_state_.set_data<CUdevice>(hpt, dev);
}

void CUDAContextManager::ctx_create_exit(hpt_t hpt, CUresult cuResult,
                                         CUcontext ctx) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_dev = entry_state_.get_data<CUdevice>(hpt);
  hp_ctx_to_device_[{std::get<0>(hpt), std::get<1>(hpt), ctx}] = entry_dev;
  hpt_ctx_stack_[hpt].push(ctx);
}

// cuCtxDestroy_(entry|exit)
void CUDAContextManager::ctx_destroy_entry(hpt_t hpt, CUcontext ctx) {
  entry_state_.set_data<CUcontext>(hpt, ctx);
}

void CUDAContextManager::ctx_destroy_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_ctx = entry_state_.get_data<CUcontext>(hpt);

  // context is no longer usuable after destroy, trying to use it should trigger
  // an error
  hp_ctx_to_device_.erase({std::get<0>(hpt), std::get<1>(hpt), entry_ctx});

  // pop thread context stack only if it's current on the calling thread
  // See https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__CTX.html
  auto &stack = thapi_at(hpt_ctx_stack_, hpt);
  if (!stack.empty() && stack.top() == entry_ctx) {
    stack.pop();
  }
}

// cuCtxSetCurrent_(entry|exit)
void CUDAContextManager::ctx_set_current_entry(hpt_t hpt, CUcontext ctx) {
  entry_state_.set_data<CUcontext>(hpt, ctx);
}

void CUDAContextManager::ctx_set_current_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_ctx = entry_state_.get_data<CUcontext>(hpt);
  // Note: don't use thapi_at, we need operator[] semantics of
  // creating if the stack does not exists yet.
  auto &stack = hpt_ctx_stack_[hpt];
  if (!stack.empty()) {
    stack.pop();
  }
  if (entry_ctx != nullptr) {
    stack.push(entry_ctx);
  }
}

// cuCtxSetCurrent_(entry|exit)
void CUDAContextManager::ctx_push_current_entry(hpt_t hpt, CUcontext ctx) {
  entry_state_.set_data<CUcontext>(hpt, ctx);
}

void CUDAContextManager::ctx_push_current_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  auto entry_ctx = entry_state_.get_data<CUcontext>(hpt);
  hpt_ctx_stack_[hpt].push(entry_ctx);
}

// cuCtxPopCurrent_exit
// Note: entry not needed
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
  hp_stream_to_ctx_[{std::get<0>(hpt), std::get<1>(hpt), cuStream}] =
      get_top_context(hpt);
}
