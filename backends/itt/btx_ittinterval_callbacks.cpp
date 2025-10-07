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
  std::unordered_map<hp_string_handle_t, std::string> itt_string_handle2name;
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

// Push instantaneous "metadata: key=value" message on the stream
static inline void push_meta_message(void *btx_handle,
                                     const char *hostname,
                                     int64_t vpid,
                                     uint64_t vtid,
                                     int64_t ts,
                                     const std::string &scope_tag,
                                     const std::string &key,
                                     const std::string &value) {
  const bool err = false;
  std::string op = "meta[" + scope_tag + "]: " + key + "=" + value;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, ts, BACKEND_ITT, op.c_str(), 1,
                              err);
}

// Attach to current task's metadata vector
static inline void attach_to_current_task_meta(data_t *state,
                                               const char *hostname,
                                               int64_t vpid,
                                               uint64_t vtid,
                                               __itt_domain *domain,
                                               const std::string &key,
                                               const std::string &value) {
  auto k = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  auto it = state->domain_handle_task_meta_stack.find(k);
  if (it == state->domain_handle_task_meta_stack.end() || it->second.empty()) {
    // Arcuable one can do
    //   push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, v);
    // Or
    // 	No active task -> fall back to thread-scoped instantaneous metadata
    return;
  }

  // Access the vector<string> in the top tuple and append "key=value"
  auto &top_tuple = it->second.top();
  std::get<2>(top_tuple).emplace_back(key + "=" + value);
}

//    _
//   /   _. | | |_   _.  _ |   _
//   \_ (_| | | |_) (_| (_ |< _>
//

// Tasks

static void lttng_ust_itt___itt_task_begin_callback(void *btx_handle,
                                                    void *usr_data,
                                                    int64_t ts,
                                                    const char *hostname,
                                                    int64_t vpid,
                                                    uint64_t vtid,
                                                    __itt_domain *domain,
                                                    __itt_id /*taskid*/,
                                                    __itt_id /*parentid*/,
                                                    __itt_string_handle *name,
                                                    char *domain__nameA_val,
                                                    char *name__strA_val) {
  auto *state = static_cast<data_t *>(usr_data);
  const auto str_name = std::string{domain__nameA_val} + ":" + std::string{name__strA_val};
  auto key = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  state->domain_handle_task_meta_stack[key].push({ts, str_name, std::vector<std::string>{}});
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

// Event

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

  std::string event_name{name_val};
  if (event_name.empty()) {
    event_name = "<unnamed event>";
  }
  state->event_id2name[{hostname, vpid, event}] = std::move(event_name);
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

// An __itt_event_end() is always matched with the nearest preceding __itt_event_start(). Otherwise,
// the __itt_event_end() call is matched with the nearest unmatched __itt_event_start() preceding
// it. Any intervening events are nested.
//
// You can nest user events of the same type or different types within each other. In the case of
// nested events, the time is considered to have been spent only in the most deeply nested user
// event region.
//
// You can overlap different ITT API events. In the case of overlapping events, the time is
// considered to have been spent only in the event region with the later __itt_event_start().
// Unmatched __itt_event_end() calls are ignored.
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

// --------------- ITT Metadata API callbacks ----------------------------------

// __itt_metadata_add(domain, id, key, type, count, data)
static void lttng_ust_itt___itt_metadata_add_callback(void *btx_handle,
                                                      void *usr_data,
                                                      int64_t ts,
                                                      const char *hostname,
                                                      int64_t vpid,
                                                      uint64_t vtid,
                                                      __itt_domain *domain,
                                                      __itt_id /*id*/,
                                                      __itt_string_handle *key,
                                                      __itt_metadata_type type,
                                                      size_t count,
                                                      void * /*data*/,
                                                      size_t /*data_len*/,
                                                      void *data_val) {
  auto *state = static_cast<data_t *>(usr_data);
  if (type == __itt_metadata_unknown || count == 0 || !data_val)
    return;

  const std::string k = state->itt_string_handle2name[{hostname, vpid, key}];
  const std::string v = format_numeric_meta(type, count, data_val);

  // No explicit scope => "belongs to the last task in the thread" (see ITT Metadata API docs).
  // Try to attach to current task; if there is no open task, emit as thread meta.
  attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, v);
}

// __itt_metadata_add_with_scope(domain, scope, key, type, count, data)
static void lttng_ust_itt___itt_metadata_add_with_scope_callback(void *btx_handle,
                                                                 void *usr_data,
                                                                 int64_t ts,
                                                                 const char *hostname,
                                                                 int64_t vpid,
                                                                 uint64_t vtid,
                                                                 __itt_domain *domain,
                                                                 __itt_scope scope,
                                                                 __itt_string_handle *key,
                                                                 __itt_metadata_type type,
                                                                 size_t count,
                                                                 void * /*data*/,
                                                                 size_t /*data_len*/,
                                                                 void *data_val) {
  auto *state = static_cast<data_t *>(usr_data);
  if (type == __itt_metadata_unknown || count == 0 || !data_val)
    return;

  const std::string k = state->itt_string_handle2name[{hostname, vpid, key}];
  const std::string v = format_numeric_meta(type, count, data_val);

  switch (scope) {
  case __itt_scope_task:
    attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, v);
    break;
  case __itt_scope_global:
    push_meta_message(btx_handle, hostname, /*vpid*/ 0, /*vtid*/ 0, ts, "global", k, v);
    break;
  default:
    // Unknown/marker/track scopes -> treating as thread (docs are unclear here).
    push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, v);
    break;
  }
}

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

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_add);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_add_with_scope);
}

#undef REGISTER_ASSOCIATED_CALLBACK
