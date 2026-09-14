#include "xprof_utils.hpp"
#include <metababel/metababel.h>

struct data_s {
  EntryState entry_state;
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
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid}, ts);
}

static void lttng_ust_pytorch_op_exit_callback(void *btx_handle,
                                               void *usr_data,
                                               int64_t ts,
                                               const char *hostname,
                                               int64_t vpid,
                                               uint64_t vtid,
                                               char *name) {
  auto *state = static_cast<data_t *>(usr_data);
  const int64_t entry_ts = state->entry_state.get_ts({hostname, vpid, vtid});

  const bool err = false;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_PYTORCH, name,
                              (ts - entry_ts), err);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  btx_register_callbacks_lttng_ust_pytorch_op_entry(btx_handle, &lttng_ust_pytorch_op_entry_callback);
  btx_register_callbacks_lttng_ust_pytorch_op_exit(btx_handle, &lttng_ust_pytorch_op_exit_callback);
}
