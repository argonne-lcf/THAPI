#include <cinttypes>
#include <cstdint>
#include <cstdio>

#include <map>
#include <string>

#include <metababel/metababel.h>

using ToggleKey = std::tuple<std::string, int64_t>;
using ToggleMap = std::map<ToggleKey, bool>;

static void init(void **data) { *data = new ToggleMap[2]; }

static void finalize(void *data) { delete[] static_cast<ToggleMap *>(data); }

static void thapi_auto_stop_callback(
    void *btx_handle, void *maps, int64_t cpuid, const char *hostname, int64_t vpid, int64_t vtid) {
  auto &auto_map = static_cast<ToggleMap *>(maps)[0];
  auto key = ToggleKey{std::string(hostname), vpid};
  /* If we have seen the auto_map trace before, we will just ignore it. */
  if (auto_map.count(key))
    return;
  /* Otherwise, we will stop tracing. */
  auto_map[key] = true;
  auto &map = static_cast<ToggleMap *>(maps)[1];
  map[key] = false;
}

static void thapi_start_callback(
    void *btx_handle, void *maps, int64_t cpuid, const char *hostname, int64_t vpid, int64_t vtid) {
  auto &map = static_cast<ToggleMap *>(maps)[1];
  auto key = ToggleKey{std::string(hostname), vpid};
  map[key] = true;
}

static void thapi_stop_callback(
    void *btx_handle, void *maps, int64_t cpuid, const char *hostname, int64_t vpid, int64_t vtid) {
  auto &map = static_cast<ToggleMap *>(maps)[1];
  auto key = ToggleKey{std::string(hostname), vpid};
  map[key] = false;
}

static void push_downstream(void *btx_handle, void *maps, const bt_message *msg) {
  bool push_msg = true;

  if (bt_message_get_type(msg) == BT_MESSAGE_TYPE_EVENT) {
    const bt_event *event = bt_message_event_borrow_event_const(msg);
    const bt_field *ccf = bt_event_borrow_common_context_field_const(event);
    const bt_field *vpid =
        ccf ? bt_field_structure_borrow_member_field_by_name_const(ccf, "vpid") : nullptr;

    /* Each trace (one per host) carries its own hostname in its environment. */
    const bt_trace *trace = bt_stream_borrow_trace_const(bt_event_borrow_stream_const(event));
    const bt_value *hostname =
        bt_trace_borrow_environment_entry_value_by_name_const(trace, "hostname");

    /* Events we cannot attribute to a process are not subject to toggling. */
    if (vpid && hostname) {
      uint64_t vpid_v = bt_field_integer_signed_get_value(vpid);
      auto &map = static_cast<ToggleMap *>(maps)[1];
      auto key = ToggleKey{std::string(bt_value_string_get(hostname)), vpid_v};
      push_msg = map[key];
    }
  }

  if (push_msg) {
    btx_push_message(btx_handle, msg);
  } else {
    bt_message_put_ref(msg);
  }
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &init);
  btx_register_callbacks_lttng_ust_toggle_auto_stop(btx_handle, &thapi_auto_stop_callback);
  btx_register_callbacks_lttng_ust_toggle_start(btx_handle, &thapi_start_callback);
  btx_register_callbacks_lttng_ust_toggle_stop(btx_handle, &thapi_stop_callback);
  btx_register_callbacks_finalize_component(btx_handle, &finalize);

  btx_register_on_downstream_message_callback(btx_handle, &push_downstream);
}
