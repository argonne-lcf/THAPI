#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include <unordered_map>
#include <vector>

// A per-thread LIFO stack of entry timestamps, not a single scalar, is
// required because PyTorch ops can re-enter themselves before returning
// (reentrance): torch.isfinite() on a complex tensor calls at::isfinite()
// again, once on the real part and once on the imaginary part, while its
// own outer call is still open (see aten/src/ATen/native/TensorCompare.cpp).
// A scalar slot would be overwritten by the inner call and the outer
// call's exit would then read the wrong (inner) entry timestamp.
struct data_s {
  std::unordered_map<hpt_t, std::vector<int64_t>> entry_stack;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

static void lttng_ust_pytorch_op_entry_callback(void *btx_handle,
                                                void *usr_data,
                                                int64_t ts,
                                                const char *hostname,
                                                int64_t vpid,
                                                uint64_t vtid,
                                                char * /*name*/) {
  static_cast<data_t *>(usr_data)->entry_stack[{hostname, vpid, vtid}].push_back(ts);
}

static void lttng_ust_pytorch_op_exit_callback(void *btx_handle,
                                               void *usr_data,
                                               int64_t ts,
                                               const char *hostname,
                                               int64_t vpid,
                                               uint64_t vtid,
                                               char *name) {
  auto *state = static_cast<data_t *>(usr_data);
  auto &stack = state->entry_stack[{hostname, vpid, vtid}];
  // Empty means an exit arrived with no matching entry (e.g. a trace
  // truncated mid-call); report it via the existing err flag instead of
  // reading undefined data.
  const bool err = stack.empty();
  const int64_t entry_ts = err ? ts : stack.back();
  if (!err) stack.pop_back();

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_PYTORCH, name,
                              (ts - entry_ts), err);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  btx_register_callbacks_lttng_ust_pytorch_op_entry(btx_handle, &lttng_ust_pytorch_op_entry_callback);
  btx_register_callbacks_lttng_ust_pytorch_op_exit(btx_handle, &lttng_ust_pytorch_op_exit_callback);
}
