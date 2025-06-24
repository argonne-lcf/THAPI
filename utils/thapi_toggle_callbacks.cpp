#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <climits>

#include <string>
#include <map>

#include <metababel/metababel.h>

using ToggleKey = std::tuple<std::string, int64_t>;
using ToggleMap = std::map<ToggleKey, bool>;

static char hostname_s[HOST_NAME_MAX + 1];

static void init(void **data) { *data = new ToggleMap; }

static void finalize(void *data) { delete static_cast<ToggleMap *>(data); }

static void thapi_start_callback(void *btx_handle, void *tmap, int64_t cpuid, const char *hostname,
                                 int64_t vpid, int64_t vtid) {
  auto map = static_cast<ToggleMap *>(tmap);
  auto key = ToggleKey{std::string(hostname), vpid};
  (*map)[key] = true;
  strncpy(hostname_s, hostname, HOST_NAME_MAX);
}

static void thapi_stop_callback(void *btx_handle, void *tmap, int64_t cpuid, const char *hostname,
                                int64_t vpid, int64_t vtid) {
  auto map = static_cast<ToggleMap *>(tmap);
  auto key = ToggleKey{std::string(hostname), vpid};
  (*map)[key] = false;
}

static void push_downstream(void *btx_handle, void *tmap, const bt_message *msg) {
  bool push_msg = true;

  if (bt_message_get_type(msg) == BT_MESSAGE_TYPE_EVENT) {
    const bt_event *event = bt_message_event_borrow_event_const(msg);
    const bt_field *ccf = bt_event_borrow_common_context_field_const(event);
    const bt_field *vpid = bt_field_structure_borrow_member_field_by_name_const(ccf, "vpid");
    uint64_t vpid_v = bt_field_integer_signed_get_value(vpid);

    auto map = static_cast<ToggleMap *>(tmap);
    auto key = ToggleKey{std::string(hostname_s), vpid_v};
    push_msg = (*map)[key];
  }

  if (push_msg) {
    btx_push_message(btx_handle, msg);
  } else {
    bt_message_put_ref(msg);
  }
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &init);
  btx_register_callbacks_lttng_ust_toggle_start(btx_handle,
                                                &thapi_start_callback);
  btx_register_callbacks_lttng_ust_toggle_stop(btx_handle,
                                               &thapi_stop_callback);
  btx_register_callbacks_finalize_component(btx_handle, &finalize);

  btx_register_on_downstream_message_callback(btx_handle, &push_downstream);
}
