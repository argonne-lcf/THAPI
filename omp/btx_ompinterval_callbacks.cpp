#include <metababel/metababel.h>

#include <unordered_map>

#include "ompt.h.include"

#include "xprof_utils.hpp"

struct data_s {
  std::unordered_map<hpt_function_name_t, uint64_t> host_start;
};

typedef struct data_s data_t;

static const char *
ompt_callback_target_data_op_name(ompt_target_data_op_t optype,
                                  const char *fallback_name) {
  switch (optype) {
  case ompt_target_data_alloc:
    return "ompt_target_data_alloc";
  case ompt_target_data_transfer_to_device:
    return "ompt_target_data_transfer_to_device";
  case ompt_target_data_transfer_from_device:
    return "ompt_target_data_transfer_from_device";
  case ompt_target_data_delete:
    return "ompt_target_data_delete";
  case ompt_target_data_associate:
    return "ompt_target_data_associate";
  case ompt_target_data_disassociate:
    return "ompt_target_data_disassociate";
  case ompt_target_data_alloc_async:
    return "ompt_target_data_alloc_async";
  case ompt_target_data_transfer_to_device_async:
    return "ompt_target_data_transfer_to_device_async";
  case ompt_target_data_transfer_from_device_async:
    return "ompt_target_data_transfer_from_device_async";
  case ompt_target_data_delete_async:
    return "ompt_target_data_delete_async";
  }
  return fallback_name;
}

static const char *ompt_callback_target_name(ompt_target_t kind,
                                             const char *fallback_name) {
  switch (kind) {
  case ompt_target:
    return "ompt_target";
  case ompt_target_enter_data:
    return "ompt_target_enter_data";
  case ompt_target_exit_data:
    return "ompt_target_exit_data";
  case ompt_target_update:
    return "ompt_target_update";
  case ompt_target_nowait:
    return "ompt_target_nowait";
  case ompt_target_enter_data_nowait:
    return "ompt_target_enter_data_nowait";
  case ompt_target_exit_data_nowait:
    return "ompt_target_exit_data_nowait";
  case ompt_target_update_nowait:
    return "ompt_target_update_nowait";
  }
  return fallback_name;
}

static void btx_initialize_component(void **usr_data) {
  *usr_data = new data_t;
}

static void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

static void _target_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *hostname, int64_t vpid, uint64_t vtid,
                             ompt_scope_endpoint_t endpoint,
                             const char *op_name) {
  auto state = static_cast<data_t *>(usr_data);
  if (endpoint == ompt_scope_begin) {
    state->host_start[{hostname, vpid, vtid, op_name}] = ts;
  } else if (endpoint == ompt_scope_end) {
    // TODO: detect error?
    const bool err = false;
    auto start_ts = state->host_start[{hostname, vpid, vtid, op_name}];
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts,
                                BACKEND_OMP, op_name, (ts - start_ts), err);
  }
}

static void ompt_callback_target_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_target_t kind,
    ompt_scope_endpoint_t endpoint, int device_num, ompt_data_t *task_data,
    ompt_id_t target_id, void *codeptr_ra) {
  const char *op_name = ompt_callback_target_name(kind, "ompt_target");
  _target_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                   op_name);
}

static void ompt_callback_target_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_target_t kind,
    ompt_scope_endpoint_t endpoint, int device_num, ompt_data_t *task_data,
    ompt_data_t *target_task_data, ompt_data_t *target_data, void *codeptr_ra) {
  const char *op_name = ompt_callback_target_name(kind, "ompt_target_emi");
  _target_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                   op_name);
}

static void _data_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                              const char *hostname, int64_t vpid, uint64_t vtid,
                              ompt_scope_endpoint_t endpoint, size_t bytes,
                              const char *op_name) {
  auto state = static_cast<data_t *>(usr_data);
  if (endpoint == ompt_scope_begin) {
    state->host_start[{hostname, vpid, vtid, op_name}] = ts;
  } else if (endpoint == ompt_scope_end) {
    // TODO: detect error?
    const bool err = false;
    auto start_ts = state->host_start[{hostname, vpid, vtid, op_name}];
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts,
                                BACKEND_OMP, op_name, (ts - start_ts), err);
    if (strcmp(op_name, "ompt_target_data_alloc") != 0) {
      btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, start_ts,
                                     BACKEND_OMP, op_name, bytes);
    }
  }
}

static void ompt_callback_target_data_op_intel_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_id_t target_id, ompt_id_t host_op_id, ompt_target_data_op_t optype,
    void *src_addr, int src_device_num, void *dest_addr, int dest_device_num,
    size_t bytes, void *codeptr_ra) {
  const char *op_name =
      ompt_callback_target_data_op_name(optype, "ompt_target_data_op_intel");
  _data_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                    bytes, op_name);
}

static void ompt_callback_target_data_op_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_data_t *target_task_data, ompt_data_t *target_data,
    ompt_id_t *host_op_id, ompt_target_data_op_t optype, void *src_addr,
    int src_device_num, void *dest_addr, int dest_device_num, size_t bytes,
    void *codeptr_ra) {
  const char *op_name =
      ompt_callback_target_data_op_name(optype, "ompt_target_data_op_emi");
  _data_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                    bytes, op_name);
}

static void ompt_callback_target_submit_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_id_t target_id, ompt_id_t host_op_id,
    unsigned int requested_num_teams) {
  const char *op_name = "ompt_target_submit";
  const bool err = false;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, ts, BACKEND_OMP,
                              op_name, 0, err);
}

static void _submit_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *hostname, int64_t vpid, uint64_t vtid,
                             ompt_scope_endpoint_t endpoint,
                             const char *op_name) {
  auto state = static_cast<data_t *>(usr_data);
  if (endpoint == ompt_scope_begin) {
    state->host_start[{hostname, vpid, vtid, op_name}] = ts;
  } else if (endpoint == ompt_scope_end) {
    // TODO: detect error?
    const bool err = false;
    auto start_ts = state->host_start[{hostname, vpid, vtid, op_name}];
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts,
                                BACKEND_OMP, op_name, (ts - start_ts), err);
  }
}

static void ompt_callback_target_submit_intel_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_id_t target_id, ompt_id_t host_op_id,
    unsigned int requested_num_teams) {
  _submit_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                   "ompt_target_submit_intel");
}

static void ompt_callback_target_submit_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_data_t *target_data, ompt_id_t *host_op_id,
    unsigned int requested_num_teams) {
  _submit_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                   "ompt_target_submit_emi");
}

#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                \
  btx_register_callbacks_lttng_ust_ompt_##base_name(btx_handle,                \
                                                    &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle,
                                              &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle,
                                            &btx_finalize_component);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_emi);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op_intel);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op_emi);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit_intel);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit_emi);
}

#undef REGISTER_ASSOCIATED_CALLBACK
