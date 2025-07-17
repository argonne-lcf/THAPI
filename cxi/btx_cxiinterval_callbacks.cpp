#include "btx_cxiinterval_callbacks.hpp"

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

// CXI
static void lttng_ust_cxi_sampling_cxi_callback(
    void *btx_handle,
    void *usr_data,
    int64_t ts,
    const char *hostname,
    int64_t /* vpid */,
    uint64_t /* vtid */,
    char *interface_name,
    char *counter,
    uint64_t value)
{
  auto *d = static_cast<data_t*>(usr_data);

  // build composite key
  NicKey key{ hostname, interface_name, counter };

  // try to insert (key -> value).  If inserted == true, this was the first sighting.
  auto [it, inserted] = d->nic_initial.emplace(key, value);
  if (inserted) {
    // first sample, just record it -- no push
    return;
  }

  // otherwise, compare to the stored first_value
  uint64_t first_value = it->second;
  if (value != first_value) {
    uint64_t diff = value - first_value;
    btx_push_message_sampling_nic(
      btx_handle,
      hostname,
      ts,
      interface_name,
      counter,
      diff);
  }
  // N.B.: leave it->second unchanged, so every push is diff vs. the original
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  // Sampling CXI
  btx_register_callbacks_lttng_ust_cxi_sampling_cxi(
      btx_handle, &lttng_ust_cxi_sampling_cxi_callback);
}
