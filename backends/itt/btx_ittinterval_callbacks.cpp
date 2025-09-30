#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include "ittnotify.h"
#include <unordered_map>
#include <string>
#include <stack>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <locale>
#include <codecvt>   // for wchar_t -> UTF-8

using hp_string_handle_t = std::tuple<hostname_t, process_id_t, __itt_string_handle *>;
using hpt_domain_handle_t = std::tuple<hostname_t, process_id_t, thread_id_t, __itt_domain *>;

using hp_event_id_t = std::tuple<hostname_t, process_id_t, uint32_t>;
using hpt_event_id_t = std::tuple<hostname_t, process_id_t, thread_id_t, uint32_t>;

struct data_s {
  std::unordered_map<hp_string_handle_t, std::string> itt_string_handle2name;
  std::unordered_map<hpt_domain_handle_t,
                     std::stack<std::tuple<std::int64_t, std::string>>> domain_handle_task_stack;
  std::unordered_map<hpt_domain_handle_t,
                     std::stack<std::vector<std::string>>> domain_handle_task_meta_stack;
  std::unordered_map<hp_event_id_t, std::string> event_id2name;
  std::unordered_map<hpt_event_id_t, std::stack<std::int64_t>> event_id_stack;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }
static void btx_finalize_component(void *usr_data) {
  auto* state = static_cast<data_t *>(usr_data);
  delete state;
}

// ---------- helpers ---------------------------------------------------------

// Safe UTF-8 from wchar_t*
static inline std::string wchars_to_utf8(const wchar_t* w, size_t length) {
  if (!w) return std::string{};
  // length may be (size_t)-1 for unknown/null-terminated in some emitters.
  size_t n = length == static_cast<size_t>(-1) ? wcslen(w) : length;
  std::wstring ws(w, w + n);
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  return conv.to_bytes(ws);
#else
  // Fallback: naive narrow (lossy on non-ASCII)
  std::string s; s.reserve(ws.size());
  for (wchar_t c : ws) s.push_back(static_cast<char>(c & 0x7F));
  return s;
#endif
}

// fetch string handle -> key name (fallback if unseen yet)
static inline std::string key_name_or_fallback(data_t* state,
                                               const char* hostname,
                                               int64_t vpid,
                                               struct __itt_string_handle* key) {
  auto it = state->itt_string_handle2name.find({hostname, vpid, key});
  if (it != state->itt_string_handle2name.end()) return it->second;
  return std::string("<unknown-key>");
}

// join helper
static inline std::string join_csv(const std::vector<std::string>& v) {
  std::ostringstream oss;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) oss << ", ";
    oss << v[i];
  }
  return oss.str();
}

// Format numeric metadata payload.
// convert to stdprintf using static hash map
static std::string format_numeric_meta(__itt_metadata_type type,
                                       size_t count,
                                       const char* data_val_bytes,
                                       size_t data_val_len) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(6);

  auto fmt_array = [&](auto tag, size_t esz, auto as_num) {
    size_t max_elems = data_val_len / esz;
    size_t n = std::min(count, max_elems);
    if (n == 0) return std::string{"[]"};
    // Copy into a properly aligned buffer before reinterpret_cast (avoid UB).
    std::vector<unsigned char> buf(data_val_bytes, data_val_bytes + n * esz);
    std::ostringstream out;
    if (n == 1) {
      out << as_num(buf.data(), 0);
    } else {
      out << "[";
      for (size_t i = 0; i < n; ++i) {
        if (i) out << ", ";
        out << as_num(buf.data(), i);
      }
      out << "]";
    }
    return out.str();
  };

  switch (type) {
    case __itt_metadata_u16:
      return fmt_array("u16", 2, [](unsigned char* p, size_t i){
        uint16_t v; std::memcpy(&v, p + i*2, 2); return static_cast<unsigned long long>(v);
      });
    case __itt_metadata_s16:
      return fmt_array("s16", 2, [](unsigned char* p, size_t i){
        int16_t v; std::memcpy(&v, p + i*2, 2); return static_cast<long long>(v);
      });
    case __itt_metadata_u32:
      return fmt_array("u32", 4, [](unsigned char* p, size_t i){
        uint32_t v; std::memcpy(&v, p + i*4, 4); return static_cast<unsigned long long>(v);
      });
    case __itt_metadata_s32:
      return fmt_array("s32", 4, [](unsigned char* p, size_t i){
        int32_t v; std::memcpy(&v, p + i*4, 4); return static_cast<long long>(v);
      });
    case __itt_metadata_u64:
      return fmt_array("u64", 8, [](unsigned char* p, size_t i){
        uint64_t v; std::memcpy(&v, p + i*8, 8); return static_cast<unsigned long long>(v);
      });
    case __itt_metadata_s64:
      return fmt_array("s64", 8, [](unsigned char* p, size_t i){
        int64_t v; std::memcpy(&v, p + i*8, 8); return static_cast<long long>(v);
      });
    case __itt_metadata_float:
      return fmt_array("f32", 4, [&](unsigned char* p, size_t i){
        float v; std::memcpy(&v, p + i*4, 4);
        std::ostringstream t; t.setf(std::ios::fixed); t << std::setprecision(6) << v; return t.str();
      });
    case __itt_metadata_double:
      return fmt_array("f64", 8, [&](unsigned char* p, size_t i){
        double v; std::memcpy(&v, p + i*8, 8);
        std::ostringstream t; t.setf(std::ios::fixed); t << std::setprecision(6) << v; return t.str();
      });
    default:
      return std::string{"<unknown-type>"};
  }
}

// Push instantaneous "metadata: key=value" message on the stream
static inline void push_meta_message(void *btx_handle,
                                     const char *hostname,
                                     int64_t vpid, uint64_t vtid,
                                     int64_t ts,
                                     const std::string& scope_tag,
                                     const std::string& key, const std::string& value) {
  const bool err = false;
  std::string op = "meta[" + scope_tag + "]: " + key + "=" + value;
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, ts,
                              BACKEND_ITT, op.c_str(), 1, err);
}

// Attach to current task's metadata vector
static inline void attach_to_current_task_meta(data_t* state,
                                               const char* hostname,
                                               int64_t vpid, uint64_t vtid,
                                               struct __itt_domain* domain,
                                               const std::string& key, const std::string& value) {
  auto k = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  auto it = state->domain_handle_task_meta_stack.find(k);
  if (it == state->domain_handle_task_meta_stack.end() || it->second.empty()) {
    // No active task -> fall back to thread-scoped instantaneous metadata
    return;
  }
  it->second.top().push_back(key + "=" + value);
}

//    _
//   /   _. | | |_   _.  _ |   _
//   \_ (_| | | |_) (_| (_ |< _>
//
static void lttng_ust_itt___itt_string_handle_create_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                __itt_string_handle* ittResult,
                                                char *str,
                                                char *str_val) {

    auto* state = static_cast<data_t *>(usr_data);
    state->itt_string_handle2name[{hostname, vpid, ittResult}] = std::string{str_val};
}

static void lttng_ust_itt___itt_task_begin_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                __itt_domain *domain,
                                                __itt_id, __itt_id, __itt_string_handle * name)  {
    auto* state = static_cast<data_t *>(usr_data);
    const auto str_name = state->itt_string_handle2name[{hostname, vpid, name}];
    auto key = hpt_domain_handle_t{hostname, vpid, vtid, domain};
    state->domain_handle_task_stack[key].push( {ts, str_name} );
    // Start a fresh metadata vector for this task FIXME should we wait to start this until first metadata emit at the task scope?
    state->domain_handle_task_meta_stack[key].push(std::vector<std::string>{});
}

static void lttng_ust_itt___itt_task_end_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                __itt_domain *domain) {
    auto* state = static_cast<data_t *>(usr_data);
    const auto key = hpt_domain_handle_t{hostname, vpid, vtid, domain};

    const auto &[start_ts, op_name] = state->domain_handle_task_stack[key].top();

    // Fetch accumulated k=v for this task
    std::string decorated_name = op_name;
    if (!state->domain_handle_task_meta_stack[key].empty()) {
      auto meta_vec = std::move(state->domain_handle_task_meta_stack[key].top());
      state->domain_handle_task_meta_stack[key].pop();
      if (!meta_vec.empty()) {
        decorated_name += " {" + join_csv(meta_vec) + "}";
      }
    }

    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                                decorated_name.c_str(), (ts - start_ts), err);

    state->domain_handle_task_stack[key].pop();
}

static void lttng_ust_itt___itt_event_create_callback(void *btx_handle, void *usr_data, int64_t /*ts*/,
                                                const char *hostname,
                                                int64_t vpid, uint64_t /*vtid*/,
                                                __itt_event event,
                                                char *name,
                                                int namelen,
                                                char *name_val)
{
    auto* state = static_cast<data_t *>(usr_data);

    std::string event_name{name_val};
    if (event_name.empty()) {
        event_name = "<unnamed event>";
    }
    state->event_id2name[{hostname, vpid, event}] = std::move(event_name);
}

static void lttng_ust_itt___itt_event_start_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                int __itt_err,
                                                __itt_event event)
{
    auto* state = static_cast<data_t *>(usr_data);
    state->event_id_stack[{hostname, vpid, vtid, event}].push(ts);
}

//An __itt_event_end() is always matched with the nearest preceding __itt_event_start(). Otherwise, the __itt_event_end() call is matched with the nearest unmatched __itt_event_start() preceding it. Any intervening events are nested.
//
//You can nest user events of the same type or different types within each other. In the case of nested events, the time is considered to have been spent only in the most deeply nested user event region.
//
//You can overlap different ITT API events. In the case of overlapping events, the time is considered to have been spent only in the event region with the later __itt_event_start(). Unmatched __itt_event_end() calls are ignored.
static void lttng_ust_itt___itt_event_end_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                int __itt_err,
                                                __itt_event event)
{
    auto* state = static_cast<data_t *>(usr_data);
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
static void lttng_ust_itt___itt_metadata_add_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                struct __itt_id /*id*/,
                                                struct __itt_string_handle *key,
                                                __itt_metadata_type type,
                                                size_t count,
                                                void * /*data*/,
                                                uint32_t data_len,
                                                char *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  if (type == __itt_metadata_unknown || count == 0 || data_len == 0 || !data_val) return;

  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = format_numeric_meta(type, count, data_val, data_len);

  // No explicit scope => "belongs to the last task in the thread" (see ITT Metadata API docs).
  // Try to attach to current task; if there is no open task, emit as thread meta.
  auto key_tuple = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  auto it = state->domain_handle_task_meta_stack.find(key_tuple);
  if (it != state->domain_handle_task_meta_stack.end() && !it->second.empty()) {
    attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, v);
  } else {
    push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, v);
  }
}

// __itt_metadata_str_addA(domain, id, key, data, length)
static void lttng_ust_itt___itt_metadata_str_addA_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                struct __itt_id /*id*/,
                                                struct __itt_string_handle *key,
                                                char * /*data*/,
                                                uint32_t length,
                                                char *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = data_val ? std::string(data_val, data_val + length) : std::string{};
  auto key_tuple = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  auto it = state->domain_handle_task_meta_stack.find(key_tuple);
  if (it != state->domain_handle_task_meta_stack.end() && !it->second.empty()) {
    attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, "\"" + v + "\"");
  } else {
    push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
  }
}

// __itt_metadata_str_addW(domain, id, key, data, length)
static void lttng_ust_itt___itt_metadata_str_addW_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                struct __itt_id /*id*/,
                                                struct __itt_string_handle *key,
                                                wchar_t * /*data*/,
                                                uint32_t length,
                                                wchar_t *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = data_val ? wchars_to_utf8(data_val, length) : std::string{};
  auto key_tuple = hpt_domain_handle_t{hostname, vpid, vtid, domain};
  auto it = state->domain_handle_task_meta_stack.find(key_tuple);
  if (it != state->domain_handle_task_meta_stack.end() && !it->second.empty()) {
    attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, "\"" + v + "\"");
  } else {
    push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
  }
}

// __itt_metadata_add_with_scope(domain, scope, key, type, count, data)
static void lttng_ust_itt___itt_metadata_add_with_scope_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                __itt_scope scope,
                                                struct __itt_string_handle *key,
                                                __itt_metadata_type type,
                                                size_t count,
                                                void * /*data*/,
                                                uint32_t data_len,
                                                char *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  if (type == __itt_metadata_unknown || count == 0 || data_len == 0 || !data_val) return;

  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = format_numeric_meta(type, count, data_val, data_len);

  switch (scope) {
    case __itt_scope_task:
      attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, v);
      break;
    case __itt_scope_thread:
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, v);
      break;
    case __itt_scope_process:
      push_meta_message(btx_handle, hostname, vpid, /*vtid*/0, ts, "process", k, v);
      break;
    case __itt_scope_global:
      push_meta_message(btx_handle, hostname, /*vpid*/0, /*vtid*/0, ts, "global", k, v);
      break;
    default:
      // Unknown/marker/track scopes -> treating as thread (docs are unclear here).
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, v);
      break;
  }
}

// __itt_metadata_str_add_with_scopeA(domain, scope, key, data, length)
static void lttng_ust_itt___itt_metadata_str_add_with_scopeA_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                __itt_scope scope,
                                                struct __itt_string_handle *key,
                                                char * /*data*/,
                                                uint32_t length,
                                                char *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = data_val ? std::string(data_val, data_val + length) : std::string{};

  switch (scope) {
    case __itt_scope_task:
      attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, "\"" + v + "\"");
      break;
    case __itt_scope_thread:
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
      break;
    case __itt_scope_process:
      push_meta_message(btx_handle, hostname, vpid, /*vtid*/0, ts, "process", k, "\"" + v + "\"");
      break;
    case __itt_scope_global:
      push_meta_message(btx_handle, hostname, /*vpid*/0, /*vtid*/0, ts, "global", k, "\"" + v + "\"");
      break;
    default:
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
      break;
  }
}

// __itt_metadata_str_add_with_scopeW(domain, scope, key, data, length)
static void lttng_ust_itt___itt_metadata_str_add_with_scopeW_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                __itt_scope scope,
                                                struct __itt_string_handle *key,
                                                wchar_t * /*data*/,
                                                uint32_t length,
                                                wchar_t *data_val) {
  auto* state = static_cast<data_t *>(usr_data);
  const std::string k = key_name_or_fallback(state, hostname, vpid, key);
  const std::string v = data_val ? wchars_to_utf8(data_val, length) : std::string{};

  switch (scope) {
    case __itt_scope_task:
      attach_to_current_task_meta(state, hostname, vpid, vtid, domain, k, "\"" + v + "\"");
      break;
    case __itt_scope_thread:
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
      break;
    case __itt_scope_process:
      push_meta_message(btx_handle, hostname, vpid, /*vtid*/0, ts, "process", k, "\"" + v + "\"");
      break;
    case __itt_scope_global:
      push_meta_message(btx_handle, hostname, /*vpid*/0, /*vtid*/0, ts, "global", k, "\"" + v + "\"");
      break;
    default:
      push_meta_message(btx_handle, hostname, vpid, vtid, ts, "thread", k, "\"" + v + "\"");
      break;
  }
}
void btx_finalize_processing(void *btx_handle, void *usr_data) {

  auto* state = static_cast<data_t *>(usr_data);
  for (auto& [key, stack] : state->event_id_stack) {
    const auto [hostname, vpid, vtid, event_id] = key;

    const auto& event_name = state->event_id2name[{hostname, vpid, event_id}];
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

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_string_handle_create);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_begin);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_end);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_create);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_start);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_event_end);

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_add);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_str_addA);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_str_addW);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_add_with_scope);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_str_add_with_scopeA);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_metadata_str_add_with_scopeW);
}

#undef REGISTER_ASSOCIATED_CALLBACK
