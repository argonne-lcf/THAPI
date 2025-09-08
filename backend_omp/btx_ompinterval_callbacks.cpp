#include "magic_enum.hpp"
#include "ompt.h.include"
#include "xprof_utils.hpp"
#include <algorithm>
#include <iostream>
#include <metababel/metababel.h>
#include <sstream>
#include <string>
#include <unordered_map>

using hpt_function_name_omp_t = std::tuple<hostname_t, process_id_t, thread_id_t, std::string>;

struct data_s {
  std::unordered_map<hpt_function_name_omp_t, uint64_t> host_start;
  std::unordered_map<std::tuple<std::string, std::string>, std::string> cached_build_name;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

// Get the start of the "ompt_scope" or return empty optional
static std::optional<int64_t> set_or_get_start(void *usr_data, hpt_function_name_omp_t key,
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

// Split on `delimiter`, return unique tokens in sorted order.
static auto split(const std::string &str, char delimiter, size_t offset = 0) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  ss.seekg(offset);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    if (!token.empty())
      tokens.push_back(token);
  }
  return tokens;
}

// Join a vector of strings with `delimiter` in between.
static std::string join(const std::vector<std::string> &v, const std::string &delimiter) {
  std::stringstream ss;
  bool first = true;
  for (const auto &s : v) {
    if (!first)
      ss << delimiter;
    first = false;
    ss << s;
  }
  return ss.str();
}

inline std::string build_name(void *usr_data, const char *event_class_name, void *) {
  // Just strip and return
  return strip_event_class_name(event_class_name);
}

// Remove element in `kind` who are in the `event_class_name`
template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
static std::string build_name(void *usr_data, const char *event_class_name, E kind) {
  // ompt_callback_target_emi, ompt_target_enter_data -> ompt_callback_target_emi:enter_data
  // ompt_callback_target_emi, ompt_target -> ompt_callback_target_emi:target
  auto state = static_cast<data_t *>(usr_data);

  // 1) Base name
  std::string base = strip_event_class_name(event_class_name);
  auto kindName = std::string{magic_enum::enum_name(kind)};

  // 2) Cache
  const auto key = std::make_tuple(base, kindName);
  auto it = state->cached_build_name.find(key);
  if (it != state->cached_build_name.end()) {
    return it->second;
  }

  // 3) Tokenize
  auto kindTokens = split(kindName, '_', strlen("ompt_"));
  auto baseTokens = split(base, '_', strlen("ompt_"));

  // 4) set_difference
  std::vector<std::string> diffTokens;
  std::set_difference(kindTokens.begin(), kindTokens.end(), baseTokens.begin(), baseTokens.end(),
                      std::back_inserter(diffTokens));

  // 5) glue it back together
  std::string kindNameProcessed;
  if (diffTokens.empty()) {
    kindNameProcessed = join(kindTokens, "_");
  } else {
    kindNameProcessed = join(diffTokens, "_");
  }

  // 6) Update Cache and Return
  it = state->cached_build_name.insert(it, {key, base + ":" + kindNameProcessed});
  return it->second;
}

template <typename EnumType = void *>
static void host_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid, ompt_scope_endpoint_t endpoint,
                             EnumType kind = EnumType()) {

  std::string op_name = build_name(usr_data, event_class_name, kind);

  if (auto start_ts = set_or_get_start(usr_data, {hostname, vpid, vtid, op_name}, endpoint, ts)) {
    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts.value(), BACKEND_OMP,
                                op_name.c_str(), (ts - start_ts.value()), err);
  }
}

static void traffic_op_callback(void *btx_handle, void *usr_data, int64_t ts,
                                const char *event_class_name, const char *hostname, int64_t vpid,
                                uint64_t vtid, ompt_scope_endpoint_t endpoint, size_t bytes,
                                ompt_target_data_op_t kind) {

  std::string op_name = build_name(usr_data, event_class_name, kind);

  if (auto start_ts = set_or_get_start(usr_data, {hostname, vpid, vtid, op_name}, endpoint, ts)) {
    btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, start_ts.value(), BACKEND_OMP,
                                   op_name.c_str(), bytes, "");
  }
}

//    _
//   /   _. | | |_   _.  _ |   _
//   \_ (_| | | |_) (_| (_ |< _>
//

// Host: We support function with 'ompt_sync_region_t'.
// We specialize with:
// - ompt_sync_region_t
// - ompt_target_t
// - ompt_target_data_op_t
static void btx_host_sync_region_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *event_class_name, const char *hostname,
                                          int64_t vpid, uint64_t vtid, ompt_sync_region_t kind,
                                          ompt_scope_endpoint_t endpoint) {

  host_op_callback(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, endpoint,
                   kind);
}

static void btx_host_target_callback(void *btx_handle, void *usr_data, int64_t ts,
                                     const char *event_class_name, const char *hostname,
                                     int64_t vpid, uint64_t vtid, ompt_target_t kind,
                                     ompt_scope_endpoint_t endpoint) {

  host_op_callback(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, endpoint,
                   kind);
}

static void btx_host_target_data_callback(void *btx_handle, void *usr_data, int64_t ts,
                                          const char *event_class_name, const char *hostname,
                                          int64_t vpid, uint64_t vtid,
                                          ompt_scope_endpoint_t endpoint,
                                          ompt_target_data_op_t optype) {

  host_op_callback(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, endpoint,
                   optype);
}

static void btx_host_callback(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name, const char *hostname, int64_t vpid,
                              uint64_t vtid, ompt_scope_endpoint_t endpoint) {

  host_op_callback(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, endpoint);
}

// Traffic

static void btx_traffic_target_data_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *event_class_name, const char *hostname,
                                             int64_t vpid, uint64_t vtid,
                                             ompt_scope_endpoint_t endpoint,
                                             ompt_target_data_op_t optype, size_t bytes) {

  traffic_op_callback(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, endpoint,
                      bytes, optype);
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

  REGISTER_ASSOCIATED_CALLBACK(btx_host_sync_region);
  REGISTER_ASSOCIATED_CALLBACK(btx_host_target);
  REGISTER_ASSOCIATED_CALLBACK(btx_host_target_data);
  REGISTER_ASSOCIATED_CALLBACK(btx_host);

  REGISTER_ASSOCIATED_CALLBACK(btx_traffic_target_data);
}

#undef REGISTER_ASSOCIATED_CALLBACK
