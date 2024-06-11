#include <metababel/metababel.h>
#include <tuple>
#include <string>
#include <mpi.h.include>
#include "xprof_utils.hpp"


struct data_s {
  EntryState entry_state;
};

typedef struct data_s data_t;

static void send_host_message(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name, const char *hostname, int64_t vpid,
                              uint64_t vtid, bool err) {

  std::string event_class_name_striped = strip_event_class_name_exit(event_class_name);
  const int64_t entry_ts = static_cast<data_t *>(usr_data)->entry_state.get_ts(
      {hostname, vpid, vtid});

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_HIP,
                              event_class_name_striped.c_str(), (ts - entry_ts), err);
}

void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) {
  delete static_cast<data_t *>(usr_data);
}

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid) {
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid},
                                                      ts);
}

static void exits_callback_mpiError_absent(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *event_class_name, const char *hostname,
                                           int64_t vpid, uint64_t vtid) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, false);
}

static void exits_callback_mpiError_present(void *btx_handle, void *usr_data, int64_t ts,
                                            const char *event_class_name, const char *hostname,
                                            int64_t vpid, uint64_t vtid, int mpiResult) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, mpiResult != MPI_SUCCESS);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  btx_register_callbacks_entries(btx_handle, &entries_callback);
  btx_register_callbacks_exits_mpiError_absent(btx_handle, &exits_callback_mpiError_absent);
  btx_register_callbacks_exits_mpiError_present(btx_handle, &exits_callback_mpiError_present);
}
