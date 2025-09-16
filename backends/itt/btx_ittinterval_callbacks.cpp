#include <queue>
#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include "ittnotify.h"
#include <unordered_map>
#include <string>

using hp_string_handle_t = std::tuple<hostname_t, process_id_t, struct __itt_string_handle *>;
using hpt_domain_handle_t = std::tuple<hostname_t, process_id_t, thread_id_t, struct __itt_domain *>;

struct data_s {
  std::unordered_map<hp_string_handle_t, std::string> ptr2name; 
  std::unordered_map<hpt_domain_handle_t, 
        std::queue<std::tuple<std::int64_t, std::string>>> host_start;
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }
static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

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
    state->ptr2name[{hostname, vpid, ittResult}] = std::string{str_val};
}

static void lttng_ust_itt___itt_task_begin_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain, struct __itt_id, struct __itt_id, struct __itt_string_handle * name)  {

    auto* state = static_cast<data_t *>(usr_data);
    const auto str_name = state->ptr2name[{hostname, vpid, name}];
    state->host_start[ {hostname, vpid, vtid, domain} ].push( {ts, str_name} );
}

static void lttng_ust_itt___itt_task_end_callback(void *btx_handle, void *usr_data, int64_t ts,
                                                const char *hostname,
                                                int64_t vpid, uint64_t vtid,
                                                struct __itt_domain *domain) {

    auto* state = static_cast<data_t *>(usr_data);
    const auto &[start_ts, op_name] = state->host_start[ {hostname, vpid, vtid, domain} ].back();

    const bool err = false;    
    btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, start_ts, BACKEND_ITT,
                                op_name.c_str(), (ts - start_ts), err);

    state->host_start[ {hostname, vpid, vtid, domain} ].pop();
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

  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_string_handle_create);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_begin);
  REGISTER_ASSOCIATED_CALLBACK(lttng_ust_itt___itt_task_end);
}

#undef REGISTER_ASSOCIATED_CALLBACK
