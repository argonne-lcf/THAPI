#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include "ittnotify.h"
#include <unordered_map>
#include <string>
#include <stack>

using hp_string_handle_t = std::tuple<hostname_t, process_id_t, struct __itt_string_handle *>;
using hpt_domain_handle_t = std::tuple<hostname_t, process_id_t, thread_id_t, struct __itt_domain *>;

using hp_event_id_t = std::tuple<hostname_t, process_id_t, uint32_t>;
using hpt_event_id_t = std::tuple<hostname_t, process_id_t, thread_id_t, uint32_t>;

struct data_s {
  std::unordered_map<hp_string_handle_t, std::string> itt_string_handle2name;
  std::unordered_map<hpt_domain_handle_t,
                     std::stack<std::tuple<std::int64_t, std::string>>> domain_handle_task_stack;
  std::unordered_map<hp_event_id_t, std::string> event_id2name;
  std::unordered_map<hpt_event_id_t, std::stack<std::int64_t>> event_id_stack;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }
static void btx_finalize_component(void *usr_data) {
  auto* state = static_cast<data_t *>(usr_data);
  delete state;
}

//    _
//   /   _. | | |_   _.  _ |   _
//   \_ (_| | | |_) (_| (_ |< _>
//
static void lttng_ust_itt___itt_string_handle_create_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_string_handle* ittResult,
                                                char *str,
                                                char *str_val) {

    auto* state = static_cast<data_t *>(usr_data);
    state->itt_string_handle2name[{hostname, vpid, ittResult}] = std::string{str_val};
}

static void lttng_ust_itt___itt_task_begin_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain,
                                                struct __itt_id, struct __itt_id, struct __itt_string_handle * name)  {
    auto* state = static_cast<data_t *>(usr_data);
    const auto str_name = state->itt_string_handle2name[{hostname, vpid, name}];
    state->domain_handle_task_stack[ {hostname, vpid, vtid, domain} ].push( {ts, str_name} );
}

static void lttng_ust_itt___itt_task_end_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain) {
    auto* state = static_cast<data_t *>(usr_data);
    const auto &[start_ts, op_name] = state->domain_handle_task_stack[ {hostname, vpid, vtid, domain} ].top();

    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                                op_name.c_str(), (ts - start_ts), err);

    state->domain_handle_task_stack[ {hostname, vpid, vtid, domain} ].pop();
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
    state->event_id2name[{hostname, vpid, event.id}] = std::move(event_name);
}

static void lttng_ust_itt___itt_event_start_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                __itt_event event)
{
    auto* state = static_cast<data_t *>(usr_data);
    state->event_id_stack[{hostname, vpid, vtid, event.id}].push(ts);
}

//An __itt_event_end() is always matched with the nearest preceding __itt_event_start(). Otherwise, the __itt_event_end() call is matched with the nearest unmatched __itt_event_start() preceding it. Any intervening events are nested.
//
//You can nest user events of the same type or different types within each other. In the case of nested events, the time is considered to have been spent only in the most deeply nested user event region.
//
//You can overlap different ITT API events. In the case of overlapping events, the time is considered to have been spent only in the event region with the later __itt_event_start(). Unmatched __itt_event_end() calls are ignored.
static void lttng_ust_itt___itt_event_end_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                __itt_event event)
{
    auto* state = static_cast<data_t *>(usr_data);
    auto &stack = state->event_id_stack[{hostname, vpid, vtid, event.id}];

    if (stack.empty()) {
        // Unmatched end is ignored per ITT semantics.
        return;
    }

    const auto start_ts = stack.top();
    stack.pop();

    const auto event_name = state->event_id2name[{hostname, vpid, event.id}];

    const bool err = false;
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                                event_name.c_str(), (ts - start_ts), err);
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
}

#undef REGISTER_ASSOCIATED_CALLBACK
