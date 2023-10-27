#include <metababel/metababel.h>

#include <unordered_map>
#include <iostream>
#include <assert.h>
#include <regex>
#include <tuple>

#include <hip.h.include>

#include "xprof_utils.hpp"
#include "btx_hipinterval_callbacks.hpp"


void send_message_lttng_host(void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name, 
                             const char *hostname, int64_t vpid, uint64_t vtid, int64_t err){
  auto *data = static_cast<data_t *>(usr_data);

  std::string event_class_name_striped = strip_event_class_name(event_class_name);
  int64_t _start = data->dispatch.at(hpt_fn_t(hostname, vpid, vtid, event_class_name_striped));

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, _start, 
                              BACKEND_HIP, event_class_name_striped.c_str(), (ts - _start), err);                     
}

void btx_initialize_component(void *btx_handle, void **usr_data) {
  *usr_data = new data_t;
}

void btx_finalize_component(void *btx_handle, void *usr_data) {
  auto *data = static_cast<data_t *>(usr_data);
  delete data;
}

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name, 
                             const char *hostname, int64_t vpid, uint64_t vtid) {
  data_t *data = (data_t *)usr_data;
  data->dispatch[hpt_fn_t(hostname, vpid, vtid, strip_event_class_name(event_class_name))] = ts;
}

static void exits_callback_hipError_absent(
  void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name, 
  const char *hostname, int64_t vpid, uint64_t vtid) {

  send_message_lttng_host(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, 0);
}

static void exits_callback_hipError_present(
  void *btx_handle, void *usr_data, int64_t ts, const char *event_class_name, 
  const char *hostname, int64_t vpid, uint64_t vtid, int64_t hipResult) {

  // Not an Error (hipResult == hipErrorNotReady)
  int64_t err = hipResult != 0 ? (hipResult == hipErrorNotReady ? 0 : hipResult) : 0; 
  send_message_lttng_host(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, err);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  btx_register_callbacks_entries(btx_handle, &entries_callback);
  btx_register_callbacks_exits_hipError_absent(btx_handle, &exits_callback_hipError_absent);
  btx_register_callbacks_exits_hipError_present(btx_handle, &exits_callback_hipError_present);
}
