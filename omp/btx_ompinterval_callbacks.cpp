#include "magic_enum.hpp"
#include "ompt.h.include"
#include "xprof_utils.hpp"
#include <iostream>
#include <metababel/metababel.h>
#include <string>
#include <unordered_map>

using hpt_function_name_omp_t =
    std::tuple<hostname_t, process_id_t, thread_id_t, std::string>;

struct data_s {
  std::unordered_map<hpt_function_name_omp_t, uint64_t> host_start;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) {
  *usr_data = new data_t;
}

static void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

// returns start_ts, for use in traffic or other messages on scope end
static int64_t _host_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                                 const char *hostname, int64_t vpid,
                                 uint64_t vtid, ompt_scope_endpoint_t endpoint,
                                 std::string const &op_name) {
  auto state = static_cast<data_t *>(usr_data);
  if (endpoint == ompt_scope_begin) {
    state->host_start[{hostname, vpid, vtid, op_name}] = ts;
  } else {
    // TODO: detect error?
    const bool err = false;
    auto start_ts = state->host_start[{hostname, vpid, vtid, op_name}];
    if (start_ts == 0) {
      start_ts = ts;
    }
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts,
                                BACKEND_OMP, op_name.c_str(), (ts - start_ts),
                                err);
    return start_ts;
  }
  return ts;
}

static void ompt_callback_target_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_target_t kind,
    ompt_scope_endpoint_t endpoint, int device_num, ompt_data_t *task_data,
    ompt_id_t target_id, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(kind));
  _host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                    op_name);
}

static void ompt_callback_target_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_target_t kind,
    ompt_scope_endpoint_t endpoint, int device_num, ompt_data_t *task_data,
    ompt_data_t *target_task_data, ompt_data_t *target_data, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(kind));
  _host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                    op_name);
}

static void _data_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                              const char *hostname, int64_t vpid, uint64_t vtid,
                              ompt_scope_endpoint_t endpoint, size_t bytes,
                              std::string const &op_name) {
  auto start_ts = _host_op_callback(btx_handle, usr_data, ts, hostname, vpid,
                                    vtid, endpoint, op_name);
  if (endpoint != ompt_scope_begin &&
      op_name.compare("ompt_target_data_alloc") == 0) {
    btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, start_ts,
                                   BACKEND_OMP, op_name.c_str(), bytes);
  }
}

static void ompt_callback_target_data_op_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_id_t target_id, ompt_id_t host_op_id,
    ompt_target_data_op_t optype, void *src_addr, int src_device_num,
    void *dest_addr, int dest_device_num, size_t bytes, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(optype));
  _data_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid,
                    ompt_scope_beginend, bytes, op_name);
}

static void ompt_callback_target_data_op_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_data_t *target_task_data, ompt_data_t *target_data,
    ompt_id_t *host_op_id, ompt_target_data_op_t optype, void *src_addr,
    int src_device_num, void *dest_addr, int dest_device_num, size_t bytes,
    void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(optype));
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

static void ompt_callback_target_submit_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
    int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
    ompt_data_t *target_data, ompt_id_t *host_op_id,
    unsigned int requested_num_teams) {
  _host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
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
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op_emi);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit_emi);
}

#undef REGISTER_ASSOCIATED_CALLBACK
