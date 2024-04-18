#include "magic_enum.hpp"
#include "ompt.h.include"
#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include <optional>
#include <string>
#include <unordered_map>

using hpt_function_name_omp_t = std::tuple<hostname_t, process_id_t, thread_id_t, std::string>;

struct data_s {
  std::unordered_map<hpt_function_name_omp_t, uint64_t> host_start;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

// Get the start of the "ompt_scope" or return empty optional
static std::optional<int64_t> set_or_gs_start(void *usr_data, hpt_function_name_omp_t key,
                                              ompt_scope_endpoint_t endpoint, int64_t ts) {
  auto state = static_cast<data_t *>(usr_data);
  switch (endpoint) {
  case ompt_scope_begin:
    state->host_start[key] = ts;
    return {};
  case ompt_scope_end:
    return state->host_start[key];
  case ompt_scope_beginend:
    return ts;
  default:
    return {};
  }
}

static void host_op_callback(void *btx_handle, void *usr_data, int64_t ts, const char *hostname,
                             int64_t vpid, uint64_t vtid, ompt_scope_endpoint_t endpoint,
                             std::string const &op_name) {
  if (auto start_ts = set_or_gs_start(usr_data, {hostname, vpid, vtid, op_name}, endpoint, ts)) {
    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts.value(), BACKEND_OMP,
                                op_name.c_str(), (ts - start_ts.value()), err);
  }
}

static void host_and_traffic_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                                         const char *hostname, int64_t vpid, uint64_t vtid,
                                         ompt_scope_endpoint_t endpoint, size_t bytes,
                                         std::string const &op_name) {
  if (auto start_ts = set_or_gs_start(usr_data, {hostname, vpid, vtid, op_name}, endpoint, ts)) {
    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts.value(), BACKEND_OMP,
                                op_name.c_str(), (ts - start_ts.value()), err);
    btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, start_ts.value(), BACKEND_OMP,
                                   op_name.c_str(), bytes);
  }
}

static void ompt_callback_target_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *hostname, int64_t vpid, uint64_t vtid,
                                          ompt_target_t kind, ompt_scope_endpoint_t endpoint,
                                          int device_num, ompt_data_t *task_data,
                                          ompt_id_t target_id, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(kind));
  host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint, op_name);
}

static void ompt_callback_target_emi_callback(void *btx_handle, void *usr_data, int64_t ts,
                                              const char *hostname, int64_t vpid, uint64_t vtid,
                                              ompt_target_t kind, ompt_scope_endpoint_t endpoint,
                                              int device_num, ompt_data_t *task_data,
                                              ompt_data_t *target_task_data,
                                              ompt_data_t *target_data, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(kind));
  host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint, op_name);
}

static void ompt_callback_target_data_op_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ompt_id_t target_id, ompt_id_t host_op_id, ompt_target_data_op_t optype, void *src_addr,
    int src_device_num, void *dest_addr, int dest_device_num, size_t bytes, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(optype));
  host_and_traffic_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, ompt_scope_beginend,
                               bytes, op_name);
}

static void ompt_callback_target_data_op_emi_callback(
    void *btx_handle, void *usr_data, int64_t ts, const char *hostname, int64_t vpid, uint64_t vtid,
    ompt_scope_endpoint_t endpoint, ompt_data_t *target_task_data, ompt_data_t *target_data,
    ompt_id_t *host_op_id, ompt_target_data_op_t optype, void *src_addr, int src_device_num,
    void *dest_addr, int dest_device_num, size_t bytes, void *codeptr_ra) {
  std::string op_name(magic_enum::enum_name(optype));
  host_and_traffic_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint, bytes,
                               op_name);
}

static void ompt_callback_target_submit_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                 const char *hostname, int64_t vpid, uint64_t vtid,
                                                 ompt_id_t target_id, ompt_id_t host_op_id,
                                                 unsigned int requested_num_teams) {
  // Nothing we can do with them, but add a callback to they are removed from the trace
}

static void ompt_callback_target_submit_emi_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                     const char *hostname, int64_t vpid,
                                                     uint64_t vtid, ompt_scope_endpoint_t endpoint,
                                                     ompt_data_t *target_data,
                                                     ompt_id_t *host_op_id,
                                                     unsigned int requested_num_teams) {
  host_op_callback(btx_handle, usr_data, ts, hostname, vpid, vtid, endpoint,
                   "ompt_target_submit_emi");
}

#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                                    \
  btx_register_callbacks_lttng_ust_ompt_##base_name(btx_handle, &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_emi);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_data_op_emi);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit);
  REGISTER_ASSOCIATED_CALLBACK(ompt_callback_target_submit_emi);
}

#undef REGISTER_ASSOCIATED_CALLBACK
