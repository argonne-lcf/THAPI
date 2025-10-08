#define INTEL_NO_MACRO_BODY
#define INTEL_ITTNOTIFY_API_PRIVATE
#include "ittnotify.h"
#include "ittnotify_config.h"

#include "xprof_utils.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <metababel/metababel.h>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using hp_string_handle_t = std::tuple<hostname_t, process_id_t, __itt_string_handle *>;
using hpt_domain_handle_t = std::tuple<hostname_t, process_id_t, thread_id_t, __itt_domain *>;

using hp_event_id_t = std::tuple<hostname_t, process_id_t, uint32_t>;
using hpt_event_id_t = std::tuple<hostname_t, process_id_t, thread_id_t, uint32_t>;

struct data_s {

  std::unordered_map<hpt_t, std::string> last_metadata_data;
  std::unordered_map<hpt_domain_handle_t,
                     std::stack<std::tuple<std::int64_t, std::string, std::vector<std::string>>>>
      domain_handle_task_meta_stack;

  std::unordered_map<hp_event_id_t, std::string> event_id2name;
  std::unordered_map<hpt_event_id_t, std::stack<std::int64_t>> event_id_stack;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }
static void btx_finalize_component(void *usr_data) {
  auto *state = static_cast<data_t *>(usr_data);
  delete state;
}

// ---------- helpers ---------------------------------------------------------

// join helper
static inline std::string join_csv(const std::vector<std::string> &v) {
  std::ostringstream oss;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i)
      oss << ", ";
    oss << v[i];
  }
  return oss.str();
}

// Format numeric metadata payload.
template <typename T> static std::string extract_and_format(size_t count, void *data_val_bytes) {

  const T *data_val = static_cast<const T *>(data_val_bytes);
  std::ostringstream oss;
  for (size_t i = 0; i < count; i++) {
    if (i > 0)
      oss << ", ";
    oss << data_val[i];
  }
  return oss.str();
}

static std::string
format_numeric_meta(__itt_metadata_type type, size_t count, void *data_val_bytes) {
  switch (type) {
  case __itt_metadata_u64:
    return extract_and_format<uint64_t>(count, data_val_bytes);
  case __itt_metadata_s64:
    return extract_and_format<int64_t>(count, data_val_bytes);
  case __itt_metadata_u32:
    return extract_and_format<uint32_t>(count, data_val_bytes);
  case __itt_metadata_s32:
    return extract_and_format<int32_t>(count, data_val_bytes);
  case __itt_metadata_u16:
    return extract_and_format<uint16_t>(count, data_val_bytes);
  case __itt_metadata_s16:
    return extract_and_format<int16_t>(count, data_val_bytes);
  case __itt_metadata_float:
    return extract_and_format<float>(count, data_val_bytes);
  case __itt_metadata_double:
    return extract_and_format<double>(count, data_val_bytes);
  default:
    return std::string{"<unknown-type>"};
  }
}
//    _
//   /   _. | | |_   _.  _ |   _
//   \_ (_| | | |_) (_| (_ |< _>
//

// --------------- ITT Metadata API callbacks ----------------------------------

static void lttng_ust_itt_metadata_metadata_callback(void *btx_handle,
                                                     void *usr_data,
                                                     int64_t ts,
                                                     const char *hostname,
                                                     int64_t vpid,
                                                     uint64_t vtid,
                                                     __itt_metadata_type type,
                                                     size_t count,
                                                     void * /*data*/,
                                                     size_t /*_data_val_length*/,
                                                     void *data_val) {

  auto *state = static_cast<data_t *>(usr_data);
  state->last_metadata_data[{hostname, vpid, vtid}] = format_numeric_meta(type, count, data_val);
}

static void lttng_ust_itt___itt_metadata_add_callback(void *btx_handle,
                                                      void *usr_data,
                                                      int64_t ts,
                                                      const char *hostname,
                                                      int64_t vpid,
                                                      uint64_t vtid,
                                                      __itt_domain *domain,
                                                      __itt_id /*id*/,
                                                      __itt_string_handle * /*key*/,
                                                      __itt_metadata_type /*type*/,
                                                      size_t /*count*/,
                                                      void * /*data*/,
                                                      char *key__strA_val) {
  auto *state = static_cast<data_t *>(usr_data);

  // If a scope is undefined, metadata belongs to the last task in the thread.
  // Try to attach to current task; if none, just return
  auto it = state->domain_handle_task_meta_stack.find({hostname, vpid, vtid, domain});
  if (it == state->domain_handle_task_meta_stack.end() || it->second.empty()) {
    return;
  }

  // Access the vector<string> in the top tuple and append "key=value"
  auto &[_a, _b, metadata_kv] = it->second.top();

  const std::string _key{key__strA_val};
  const std::string value = state->last_metadata_data[{hostname, vpid, vtid}];

  metadata_kv.emplace_back(_key + "=" + value);
}

// --------------- ITT Task API callbacks ----------------------------------

static void lttng_ust_itt___itt_task_begin_callback(void *btx_handle,
                                                    void *usr_data,
                                                    int64_t ts,
                                                    const char *hostname,
                                                    int64_t vpid,
                                                    uint64_t vtid,
                                                    __itt_domain *domain,
                                                    __itt_id /*taskid*/,
                                                    __itt_id /*parentid*/,
                                                    __itt_string_handle * /*name*/,
                                                    char *domain__nameA_val,
                                                    char *name__strA_val) {
  auto *state = static_cast<data_t *>(usr_data);
  const auto str_name = std::string{domain__nameA_val} + ":" + std::string{name__strA_val};
  state->domain_handle_task_meta_stack[{hostname, vpid, vtid, domain}].emplace(
      ts, str_name, std::vector<std::string>{});
}

static void lttng_ust_itt___itt_task_end_callback(void *btx_handle,
                                                  void *usr_data,
                                                  int64_t ts,
                                                  const char *hostname,
                                                  int64_t vpid,
                                                  uint64_t vtid,
                                                  __itt_domain *domain) {
  auto *state = static_cast<data_t *>(usr_data);
  const auto key = hpt_domain_handle_t{hostname, vpid, vtid, domain};

  const auto &[start_ts, op_name, meta_vec] = state->domain_handle_task_meta_stack[key].top();

  std::string decorated_name = op_name;
  if (!meta_vec.empty()) {
    decorated_name += " {" + join_csv(meta_vec) + "}";
  }

  const bool err = false;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                              decorated_name.c_str(), (ts - start_ts), err);

  state->domain_handle_task_meta_stack[key].pop();
}

// --------------- ITT Event API callbacks ----------------------------------
//
// Events, are like tasks but they can be without an end
// In this case, they are just tik mark
static void lttng_ust_itt___itt_event_create_callback(void *btx_handle,
                                                      void *usr_data,
                                                      int64_t /*ts*/,
                                                      const char *hostname,
                                                      int64_t vpid,
                                                      uint64_t /*vtid*/,
                                                      __itt_event event,
                                                      char *name,
                                                      int namelen,
                                                      char *name_val) {
  auto *state = static_cast<data_t *>(usr_data);
  state->event_id2name[{hostname, vpid, event}] = std::string{name_val};
}

static void lttng_ust_itt___itt_event_start_callback(void *btx_handle,
                                                     void *usr_data,
                                                     int64_t ts,
                                                     const char *hostname,
                                                     int64_t vpid,
                                                     uint64_t vtid,
                                                     int __itt_err,
                                                     __itt_event event) {
  auto *state = static_cast<data_t *>(usr_data);
  state->event_id_stack[{hostname, vpid, vtid, event}].push(ts);
}

static void lttng_ust_itt___itt_event_end_callback(void *btx_handle,
                                                   void *usr_data,
                                                   int64_t ts,
                                                   const char *hostname,
                                                   int64_t vpid,
                                                   uint64_t vtid,
                                                   int __itt_err,
                                                   __itt_event event) {
  auto *state = static_cast<data_t *>(usr_data);
  auto &stack = state->event_id_stack[{hostname, vpid, vtid, event}];

  if (stack.empty()) {
    // Unmatched end is ignored per ITT semantics.
    return;
  }

  const auto start_ts = stack.top();
  stack.pop();

  const auto event_name = state->event_id2name[{hostname, vpid, event}];

  const bool err = false;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                              event_name.c_str(), (ts - start_ts), err);
}

// --------------- Finalization ----------------------------------
void btx_finalize_processing(void *btx_handle, void *usr_data) {

  auto *state = static_cast<data_t *>(usr_data);
  for (auto &[key, stack] : state->event_id_stack) {
    const auto [hostname, vpid, vtid, event_id] = key;

    const auto &event_name = state->event_id2name[{hostname, vpid, event_id}];
    while (!stack.empty()) {
      const auto start_ts = stack.top();
      stack.pop();

      const bool err = false;
      btx_push_message_lttng_host(btx_handle, hostname.c_str(), vpid, vtid, start_ts, BACKEND_ITT,
                                  event_name.c_str(), 1, err);
    }
  }
}
//    _                             _
//   |_)  _   _  o  _ _|_  _  ._   /   _. | | |_   _.  _ |   _
//   | \ (/_ (_| | _>  |_ (/_ |    \_ (_| | | |_) (_| (_ |< _>
//            _|

#define REGISTER_ASSOCIATED_CALLBACK(base_name)                                                    \
  btx_register_callbacks_##base_name(btx_handle, &base_name##_callback);

void btx_register_usr_callbacks(void *btx_handle) {

  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  btx_register_callbacks_finalize_processing(btx_handle, &btx_finalize_processing);

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_begin);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_end);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_create);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_start);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_end);

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt_metadata_metadata);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_add);
}

#undef REGISTER_ASSOCIATED_CALLBACK
