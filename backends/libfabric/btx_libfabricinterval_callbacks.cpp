#include "xprof_utils.hpp"
#include <metababel/metababel.h>
#include <rdma/fi_errno.h>

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

/* Separate pending calls by host, process, thread, and function. */
using CallKey =
    std::tuple<std::string, int64_t, uint64_t, std::string>;

struct State {
    std::map<CallKey, std::vector<int64_t>> pending;
};

static void initialize(void **usr_data)
{
    *usr_data = new State;
}

static void finalize(void *usr_data)
{
    delete static_cast<State *>(usr_data);
}

static void entry_callback(void *btx_handle,
                           void *usr_data,
                           int64_t ts,
                           const char *event_class_name,
                           const char *hostname,
                           int64_t vpid,
                           uint64_t vtid)
{
    (void)btx_handle;

    auto *state = static_cast<State *>(usr_data);
    std::string name = strip_event_class_name_entry(event_class_name);

    CallKey key{hostname, vpid, vtid, name};
    state->pending[key].push_back(ts);
}

static void exit_callback(void *btx_handle,
                          void *usr_data,
                          int64_t ts,
                          const char *event_class_name,
                          const char *hostname,
                          int64_t vpid,
                          uint64_t vtid,
                          ssize_t ret)
{
    auto *state = static_cast<State *>(usr_data);
    std::string name = strip_event_class_name_exit(event_class_name);

    CallKey key{hostname, vpid, vtid, name};
    auto it = state->pending.find(key);

    /* An exit without a recorded entry cannot define a duration. */
    if (it == state->pending.end())
        return;

    int64_t start = it->second.back();
    it->second.pop_back();

    if (it->second.empty())
        state->pending.erase(it);

    if (ts < start)
        return;

    /*
     * Keep retry/empty-poll intervals visible, but do not classify
     * FI_EAGAIN as a hard error.
     */
    bool error = ret < 0 && ret != -FI_EAGAIN;

    btx_push_message_lttng_host(
        btx_handle,
        hostname,
        vpid,
        vtid,
        start,
        BACKEND_LIBFABRIC,
        name.c_str(),
        static_cast<uint64_t>(ts - start),
        error
    );
}

void btx_register_usr_callbacks(void *btx_handle)
{
    btx_register_callbacks_initialize_component(btx_handle, &initialize);
    btx_register_callbacks_finalize_component(btx_handle, &finalize);
    btx_register_callbacks_entries(btx_handle, &entry_callback);
    btx_register_callbacks_exits(btx_handle, &exit_callback);
}