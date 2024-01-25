#include "context_manager.hpp"


bool CUDAContextManager::get_device(hpt_t hpt, CUdevice *pdev) {
  CUcontext ctx;
  bool found = get_top_context(hpt, &ctx);
  if (!found) {
    return false;
  }
  // std::cerr << "get device for thread " << std::get<2>(hpt) << std::endl;
  auto dev_find = hp_ctx_to_device_.find({std::get<0>(hpt), std::get<1>(hpt), ctx});
  if (dev_find != hp_ctx_to_device_.end()) {
    *pdev = dev_find->second;
    return true;
  }
  return false;
}

bool CUDAContextManager::get_top_context(hpt_t hpt, CUcontext *pctx) {
  try {
    // std::cerr << "get top ctx for thread " << std::get<2>(hpt) << std::endl;
    auto& hpt_stack = hpt_ctx_stack_.at(hpt);
    // Note: the primary context is not used automatically, it must be pushed or set
    // current first and be on the stack
    if (hpt_stack.empty()) {
      // std::cerr << "empty stack for thread " << std::get<2>(hpt) << std::endl;
      return false;
    }
    *pctx = hpt_stack.top();
    return true;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt, "no context stack for thread");
  }
  return false;
}

bool CUDAContextManager::get_stream_device(hpt_t hpt, CUstream stream, CUdevice *pdev) {
  if (stream == 0) {
    // using the default stream in the top context on the stack
    return get_device(hpt, pdev);
  }
  hp_stream_t hp_stream_key = {std::get<0>(hpt), std::get<1>(hpt), stream};
  CUcontext ctx;
  try {
    ctx = hp_stream_to_ctx_.at(hp_stream_key);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
      "no context stack for thread in CUDAContextManager::get_stream_device");
  }
  auto ctx_find = hp_ctx_to_device_.find({std::get<0>(hpt), std::get<1>(hpt), ctx});
  if (ctx_find != hp_ctx_to_device_.end()) {
    *pdev = ctx_find->second;
    return true;
  }
  return false;
}



// cuPrimaryCtxRetain_v2_(entry|exit)
void CUDAContextManager::primary_ctx_retain_entry(hpt_t hpt, CUdevice dev) {
  // std::cerr << "primaryctx retain entry thread " << std::get<2>(hpt) << std::endl;
  hpt_entry_dev_[hpt] = dev;
}

void CUDAContextManager::primary_ctx_retain_exit(hpt_t hpt, CUresult cuResult,
                                                 CUcontext ctx) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  // std::cerr << "primaryctx retain exit thread " << std::get<2>(hpt) << std::endl;
  try {
    auto entry_dev = hpt_entry_dev_.at(hpt);
    hp_device_primary_ctx_[{std::get<0>(hpt), std::get<1>(hpt), entry_dev}] = ctx;
    hp_ctx_to_device_[{std::get<0>(hpt), std::get<1>(hpt), ctx}] = entry_dev;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry device in primary ctx retain exit callback");
  }
}


// cuPrimaryCtxRelease_v2_(entry|exit)
void CUDAContextManager::primary_ctx_release_entry(hpt_t hpt, CUdevice dev) {
  hpt_entry_dev_[hpt] = dev;
}

void CUDAContextManager::primary_ctx_release_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  CUdevice entry_dev;
  try {
    entry_dev = hpt_entry_dev_.at(hpt);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry device in primary ctx release exit callback");
  }
  // std::cerr << "release primary ctx for device " << entry_dev << " thread " << std::get<2>(hpt) << std::endl;
  hp_device_t hp_dev_key = {std::get<0>(hpt), std::get<1>(hpt), entry_dev};
  try {
    auto ctx = hp_device_primary_ctx_.at(hp_dev_key);
    hp_device_primary_ctx_.erase(hp_dev_key);
    hp_ctx_to_device_.erase({std::get<0>(hpt), std::get<1>(hpt), ctx});
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no context found in primary ctx release exit callback");
  }
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
  CUdevice entry_dev;
  try {
    entry_dev = hpt_entry_dev_.at(hpt);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry device for thread in ctx create exit callback");
  }
  if (hpt_ctx_stack_.count(hpt) == 0) {
    hpt_ctx_stack_[hpt] = {};
  }
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
    entry_ctx = hpt_entry_ctx_.at(hpt);
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
    auto& stack = hpt_ctx_stack_.at(hpt);
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
  // std::cerr << "set current exit thread " << std::get<2>(hpt) << std::endl;
  CUcontext entry_ctx;
  try {
    entry_ctx = hpt_entry_ctx_.at(hpt);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry context for thread in ctx set current exit callback");
  }
  try {
    if (hpt_ctx_stack_.count(hpt) == 0) {
      // std::cerr << "init stack for thread " << std::get<2>(hpt) << std::endl;
      hpt_ctx_stack_[hpt] = {};
    }
    auto& stack = hpt_ctx_stack_.at(hpt);
    if (!stack.empty()) {
      stack.pop();
    }
    stack.push(entry_ctx);
    // std::cerr << "stack size " << std::get<2>(hpt) << " " << hpt_ctx_stack_[hpt].size() << std::endl;
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no context stack for thread in ctx set current exit callback");
  }
}

// cuCtxSetCurrent_(entry|exit)
void CUDAContextManager::ctx_push_current_entry(hpt_t hpt, CUcontext ctx) {
  hpt_entry_ctx_[hpt] = ctx;
}

void CUDAContextManager::ctx_push_current_exit(hpt_t hpt, CUresult cuResult) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  try {
    if (hpt_ctx_stack_.count(hpt) == 0) {
      hpt_ctx_stack_[hpt] = {};
    }
    auto entry_ctx = hpt_entry_ctx_.at(hpt);
    hpt_ctx_stack_[hpt].push(entry_ctx);
  } catch(const std::out_of_range& oor) {
    THAPI_FATAL_HPT(hpt,
                    "no entry context for thread in ctx push current exit callback");
  }
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
  // std::cerr << "pop current on thread " << std::get<2>(hpt) << std::endl;
  hpt_ctx_stack_[hpt].pop();
  // assert(popped_ctx == cuContext);
}

// cuStreamCreate_exit
// Note: entry not needed
void CUDAContextManager::stream_create_exit(hpt_t hpt, CUresult cuResult,
                                            CUstream cuStream) {
  if (cuResultIsError(cuResult)) {
    return;
  }
  CUcontext ctx;
  bool found = get_top_context(hpt, &ctx);
  if (!found) {
    // TODO: how to we raise error here? Should not reach
  }
  hp_stream_t hp_stream_key = {std::get<0>(hpt), std::get<1>(hpt), cuStream}; 
  
  hp_stream_to_ctx_[hp_stream_key] = ctx;
}
