#include <metababel/metababel.h>

#include <assert.h>
#include <iostream>
#include <regex>
#include <tuple>
#include <string>
#include <unordered_map>

#include <hip.h.include>
#include "xprof_utils.hpp"


struct data_s {
  std::unordered_map<hpt_function_name_t, int64_t> dispatch;
};

typedef struct data_s data_t;

std::string strip_event_class_name(const char *str) {
  std::string temp(str);
  std::smatch match;
  std::regex_search(temp, match, std::regex(":(.*?)_?(?:entry|exit)?$"));

  // The entire match is hold in the first item, sub_expressions 
  // (parentheses delimited groups) are stored after.
  assert( match.size() > 1 && "Event_class_name not matching regex.");

  return match[1].str();
}

static void send_host_message(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name, const char *hostname, int64_t vpid,
                              uint64_t vtid, bool err) {

  std::string event_class_name_striped = strip_event_class_name(event_class_name);
  const int64_t _start = (static_cast<data_t *>(usr_data))->dispatch.at({hostname, vpid, vtid, event_class_name_striped});

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, _start, BACKEND_HIP,
                              event_class_name_striped.c_str(), (ts - _start), err);
}

void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid) {

  ((data_t *)usr_data)->dispatch[{hostname, vpid, vtid, strip_event_class_name(event_class_name)}] = ts;
}

static void exits_callback_hipError_absent(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *event_class_name, const char *hostname,
                                           int64_t vpid, uint64_t vtid) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, false);
}

static void exits_callback_hipError_present(void *btx_handle, void *usr_data, int64_t ts,
                                            const char *event_class_name, const char *hostname,
                                            int64_t vpid, uint64_t vtid, hipError_t hipResult) {

  // Not an Error (hipResult == hipErrorNotReady)
  bool err = (hipResult != hipSuccess) && (hipResult != hipErrorNotReady);
  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, err);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  btx_register_callbacks_entries(btx_handle, &entries_callback);
  btx_register_callbacks_exits_hipError_absent(btx_handle, &exits_callback_hipError_absent);
  btx_register_callbacks_exits_hipError_present(btx_handle, &exits_callback_hipError_present);
}
