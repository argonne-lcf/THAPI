#include "tally_core.hpp"
#include "xprof_utils.hpp"
#include <cstdint>
#include <metababel/metababel.h>
#include <unordered_map>

using name_t = std::string;
using metadata_t = std::string;
using _hptbn_t = std::tuple<hostname_t, process_id_t, thread_id_t, backend_t, name_t>;

using _hptbnm_t = std::tuple<hostname_t, process_id_t, thread_id_t, backend_t, name_t, metadata_t>;
using _hptn_dsb_nm_t = std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id,
                                  thapi_device_id, name_t, metadata_t>;

struct aggreg_s {
  std::unordered_map<_hptbn_t, TallyCoreBase> host;
  std::unordered_map<_hptbnm_t, TallyCoreBase> traffic;
  std::unordered_map<_hptn_dsb_nm_t, TallyCoreBase> device;
  bool discard_metadata;
};
using aggreg_t = struct aggreg_s;

static void initialize_component_callback(void **usr_data) { *usr_data = new aggreg_t; }

static void read_params_callback(void *usr_data, btx_params_t *usr_params) {
  auto *data = static_cast<aggreg_t *>(usr_data);
  data->discard_metadata = !!usr_params->discard_metadata;
}

static void finalize_component_callback(void *usr_data) {
  auto *data = static_cast<aggreg_t *>(usr_data);
  delete data;
}

static void finalize_processing_callback(void *btx_handle, void *usr_data) {
  auto *data = static_cast<aggreg_t *>(usr_data);
  // Host
  for (const auto &[hptb_function_name, t] : data->host) {
    const auto [hostname, vpid, vtid, backend_id, name] = hptb_function_name;
    btx_push_message_aggreg_host(btx_handle, hostname.c_str(), vpid, vtid, name.c_str(), t.min,
                                 t.max, t.duration, t.count, (int64_t)backend_id, t.error);
  }
  // Traffic
  for (const auto &[hptb_function_name, t] : data->traffic) {
    const auto &[hostname, vpid, vtid, backend_id, name, metadata] = hptb_function_name;
    btx_push_message_aggreg_traffic(btx_handle, hostname.c_str(), vpid, vtid, name.c_str(), t.min,
                                    t.max, t.duration, t.count, (int64_t)backend_id,
                                    metadata.c_str());
  }

  // Device
  for (const auto &[hptb_function_name, t] : data->device) {
    const auto &[hostname, vpid, vtid, did, sdid, name, metadata] = hptb_function_name;
    btx_push_message_aggreg_device(btx_handle, hostname.c_str(), vpid, vtid, name.c_str(), t.min,
                                   t.max, t.duration, t.count, did, sdid, metadata.c_str());
  }
}

static void host_usr_callback(void *btx_handle, void *usr_data, const char *hostname, int64_t vpid,
                              uint64_t vtid, int64_t ts, int64_t backend_id, const char *name,
                              uint64_t dur, bt_bool err) {
  auto *data = static_cast<aggreg_t *>(usr_data);
  data->host[{hostname, vpid, vtid, (backend_t)backend_id, name}] += {dur, (bool)err};
}

static void traffic_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                 int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend_id,
                                 const char *name, uint64_t size, const char *metadata) {

  auto *data = static_cast<aggreg_t *>(usr_data);
  if (data->discard_metadata)
    data->traffic[{hostname, vpid, vtid, (backend_t)backend_id, name, "discarded_metadata"}] +=
        {size, false};
  else
    data->traffic[{hostname, vpid, vtid, (backend_t)backend_id, name, metadata}] += {size, false};
}

static void device_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend_id,
                                const char *name, uint64_t dur, uint64_t did, uint64_t sdid,
                                bt_bool err, const char *metadata) {

  auto *data = static_cast<aggreg_t *>(usr_data);
  if (data->discard_metadata)
    data->device[{hostname, vpid, vtid, did, sdid, name, "discarded_metadata"}] += {dur, (bool)err};
  else
    data->device[{hostname, vpid, vtid, did, sdid, name, metadata}] += {dur, (bool)err};
}

// Merge all the aggreg messages in the same trace
static void aggreg_host_callback(void *btx_handle, void *usr_data, const char *hostname,
                                 int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                 uint64_t max, uint64_t total, uint64_t count, uint64_t backend,
                                 uint64_t err) {

  btx_push_message_aggreg_host(btx_handle, hostname, vpid, vtid, name, min, max, total, count,
                               (int64_t)backend, err);
}

static void aggreg_device_callback(void *btx_handle, void *usr_data, const char *hostname,
                                   int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                   uint64_t max, uint64_t total, uint64_t count, uint64_t did,
                                   uint64_t sdid, const char *metadata) {

  btx_push_message_aggreg_device(btx_handle, hostname, vpid, vtid, name, min, max, total, count,
                                 did, sdid, metadata);
}

static void aggreg_traffic_callback(void *btx_handle, void *usr_data, const char *hostname,
                                    int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                    uint64_t max, uint64_t total, uint64_t count, uint64_t backend,
                                    const char *metadata) {

  btx_push_message_aggreg_traffic(btx_handle, hostname, vpid, vtid, name, min, max, total, count,
                                  (int64_t)backend, metadata);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &initialize_component_callback);
  btx_register_callbacks_read_params(btx_handle, &read_params_callback);

  btx_register_callbacks_finalize_processing(btx_handle, &finalize_processing_callback);
  btx_register_callbacks_finalize_component(btx_handle, &finalize_component_callback);

  btx_register_callbacks_lttng_host(btx_handle, &host_usr_callback);
  btx_register_callbacks_lttng_traffic(btx_handle, &traffic_usr_callback);
  btx_register_callbacks_lttng_device(btx_handle, &device_usr_callback);

  btx_register_callbacks_aggreg_host(btx_handle, &aggreg_host_callback);
  btx_register_callbacks_aggreg_device(btx_handle, &aggreg_device_callback);
  btx_register_callbacks_aggreg_traffic(btx_handle, &aggreg_traffic_callback);
}
