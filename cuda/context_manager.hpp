#pragma once

#include <stack>
#include <unordered_map>

#include "btx_cudainterval_callbacks.hpp"
#include "entry_state.hpp"

/** CUDAContextManager
 *
 * Keep track of which device is current on each (hostname, process, thread)
 * and provide method `hpt_get_device` for use by the rest of the interval
 * callback code. This can be used to associate a device when an operation
 * like memcpy or a kernel launch runs that must be on a specific device.
 *
 * This is needed because the stream of driver API calls we get in THAPI does
 * not pass the device and stream to all API calls - it is hidden inside the
 * thread local context in the CUDA runtime. To deal with this, we need to
 * intercept the driver API context management calls and save the device and
 * stream, so that when kernels, memcpy, and other operations are launched we
 * know which device and stream were used.
 *
 * Background information:
 *
 * Most CUDA code is written using the high level runtime API, which calls the
 * driver API to implement all it's functionality. The driver API can also be
 * used directly but this is typically only common in library code. The API
 * calls seen by THAPI are driver API calls only, we do not see the high level
 * runtime API calls that generated the driver API calls.
 *
 * The CUDA programming model uses per device contexts, which keep state like
 * memory allocations and current stream. Each thread has a hidden thread local
 * context stack to manage which context will be used by the next API call in that
 * thread. Things like cudaSetDevice in the runtime API will change the context for
 * the current thread, which we see in thapi in the underlying driver API calls
 * made to implement that functionality.
 *
 * The CUDA runtime API uses a single "primary context" per device per process.
 * For code written with the runtime API, this is typically the only context
 * used, and it will be set/pushed/poped on the per thread context stack as
 * needed by various other API calls. When a new thread is created and a runtime
 * API call made, the runtime API will make sure that the primary context is set
 * as the context for that new thread.
 *
 * For code written using purely the driver API or a mix of APIs, contexts can
 * also be created with cuCtxCreate and then pushed/poped/set as the context
 * for different threads (but more typcially would only be used by a single
 * thread and then destoryed).
 *
 * See also:
 * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__PRIMARY__CTX.html
 * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__CTX.html
 *
 * A typical flow of context APIs for a simple application using the runtime
 * API only looks like this:
 *
 * Setup: (associate device with context?)
 * 1. cuCtxGetCurrent_(entry|exit)
 * 2. cuCtxSetCurrent_(entry|exit)
 * 3. cuDevicePrimaryCtxRetain_(entry|exit)
 * 4. cuCtxGetCurrent_(entry|exit)
 * 5. cuCtxGetDevice_(entry|exit)
 *
 * Kernel launch:
 *   1. cuCtxPushCurrent_v2_(entry|exit)
 *   2. cuLibraryGetModule
 *   3. cuCtxPopCurrent_v2_(entry|exit)
 *
 * Cleanup:
 *   1. cuDevicePrimaryCtxRelease_(entry|exit)
 */
class CUDAContextManager {
public:
  CUDAContextManager(EntryState& entry_state) : entry_state_{entry_state}
  {}

  // returns true and sets output var if found, false otherwise
  CUdevice get_device(hpt_t hpt);

  // returns true and sets output var if found, false otherwise
  CUcontext get_top_context(hpt_t hpt);

  // Note: streams associate with a context (and a device) at create time; at time
  // of use, they may be using a device other than the one associated with the
  // thread's top context stack.
  // returns true and sets output var if found, false otherwise.
  // if stream is 0 / default, fallback to get_device (top context device)
  CUdevice get_stream_device(hpt_t hpt, CUstream stream);

  // save dev
  void primary_ctx_retain_entry(hpt_t hpt, CUdevice dev);
  void primary_ctx_retain_exit(hpt_t hpt, CUresult cuResult, CUcontext ctx);

  void primary_ctx_release_entry(hpt_t hpt, CUdevice dev);
  void primary_ctx_release_exit(hpt_t hpt, CUresult cuResult);

  void primary_ctx_reset_entry(hpt_t hpt, CUdevice dev);
  void primary_ctx_reset_exit(hpt_t hpt, CUresult cuResult);

  void ctx_create_entry(hpt_t hpt, CUdevice dev);
  void ctx_create_exit(hpt_t hpt, CUresult cuResult, CUcontext ctx);

  void ctx_destroy_entry(hpt_t hpt, CUcontext ctx);
  void ctx_destroy_exit(hpt_t hpt, CUresult cuResult);

  void ctx_set_current_entry(hpt_t hpt, CUcontext ctx);
  void ctx_set_current_exit(hpt_t hpt, CUresult cuResult);

  void ctx_push_current_entry(hpt_t hpt, CUcontext ctx);
  void ctx_push_current_exit(hpt_t hpt, CUresult cuResult);

  void ctx_pop_current_entry(hpt_t hpt);
  void ctx_pop_current_exit(hpt_t hpt, CUresult cuResult, CUcontext ctx);

  // Note: entry callback not needed
  void stream_create_exit(hpt_t hpt, CUresult cuResult, CUstream phStream_val);

private:
  // There is at most once primary context per (host, process, device)
  std::unordered_map<hp_device_t, CUcontext> hp_device_primary_ctx_;

  // Contexts must be on the per thread stack to be used, even primary contexts
  std::unordered_map<hpt_t, context_stack_t> hpt_ctx_stack_;

  // Contexts can be used by any thread in a process, regardless of which
  // thread they were created in, so they are not dependent on thread id
  std::unordered_map<hp_context_t, CUdevice> hp_ctx_to_device_;

  // Track streams so when they are passed to API calls we know what context
  // (and thereby what device) the API call is using
  std::unordered_map<hp_stream_t, CUcontext> hp_stream_to_ctx_;

  // Need to save device and context between entry/exit callbacks. Re-use same entry
  // state instance to save memory.
  EntryState& entry_state_;
};
