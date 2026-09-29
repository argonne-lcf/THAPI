#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include <string>
#include <unordered_map>
#include <vector>

// PyTorch requies a per-thread LIFO stack since there is reentrace and nesting calls.
struct data_s {
  std::unordered_map<hpt_t, std::vector<int64_t>> entry_stack;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

// PyTorch identifies an operator by TWO strings: a schema name (e.g.
// "aten::abs") and an overload name (e.g. "" for the default overload, "out"
// Joining them here keeps e.g. "aten::abs" and "aten::abs.out" distinguishable
// in the trace instead of both showing up as plain "aten::abs".
static std::string qualified_name(const char *name, const char *overload_name) {
  return (overload_name[0] == '\0') ? name : std::string(name) + "." + overload_name;
}

static void lttng_ust_pytorch_op_entry_callback(void *btx_handle,
                                                void *usr_data,
                                                int64_t ts,
                                                const char *hostname,
                                                int64_t vpid,
                                                uint64_t vtid,
                                                char * /*name*/,
                                                char * /*overload_name*/) {
  static_cast<data_t *>(usr_data)->entry_stack[{hostname, vpid, vtid}].push_back(ts);
}

static void lttng_ust_pytorch_op_exit_callback(void *btx_handle,
                                               void *usr_data,
                                               int64_t ts,
                                               const char *hostname,
                                               int64_t vpid,
                                               uint64_t vtid,
                                               char *name,
                                               char *overload_name) {
  auto *state = static_cast<data_t *>(usr_data);
  auto &stack = state->entry_stack[{hostname, vpid, vtid}];

  // Empty means an exit arrived with no matching entry
  const bool err = stack.empty();
  int64_t entry_ts = ts;
  if (!err) {
    entry_ts = stack.back();
    stack.pop_back();
  }

  const std::string full_name = qualified_name(name, overload_name);
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_PYTORCH,
                              full_name.c_str(), (ts - entry_ts), err);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  btx_register_callbacks_lttng_ust_pytorch_op_entry(btx_handle,
                                                    &lttng_ust_pytorch_op_entry_callback);
  btx_register_callbacks_lttng_ust_pytorch_op_exit(btx_handle, &lttng_ust_pytorch_op_exit_callback);
}
