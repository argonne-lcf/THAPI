#include <metababel/metababel.h>

static void init(void **data) { *data = calloc(1, sizeof(int)); }

static void finalize(void *data) { free(data); }

static void thapi_start_callback(void *btx_handle, void *push, long int cpuid,
                                 int vpid, int vtid) {
  *((int *)push) = 1;
}

static void thapi_stop_callback(void *btx_handle, void *push, long int cpuid,
                                int vpid, int vtid) {
  *((int *)push) = 0;
}

static void push_downstream(void *btx_handle, void *push,
                            const bt_message *msg) {
  if (*((int *)push) == 1)
    btx_push_message(btx_handle, msg);
  else
    bt_message_put_ref(msg);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &init);
  btx_register_callbacks_finalize_component(btx_handle, &finalize);

  btx_register_callbacks_lttng_ust_toggle_start(btx_handle,
                                                &thapi_start_callback);
  btx_register_callbacks_lttng_ust_toggle_stop(btx_handle,
                                               &thapi_stop_callback);

  btx_register_on_downstream_message_callback(btx_handle, &push_downstream);
}
