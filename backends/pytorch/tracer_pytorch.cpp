#include <ATen/record_function.h>

#include "pytorch.h"

// ENTRY: fires BEFORE the op runs. LTTng adds time + vpid/vtid via context.
static std::unique_ptr<at::ObserverContext> on_entry(const at::RecordFunction &fn) {
  tracepoint(lttng_ust_pytorch, op_entry, fn.name());
  return nullptr;
}

// EXIT: fires AFTER the op returns.
static void on_exit(const at::RecordFunction &fn, at::ObserverContext *) {
  tracepoint(lttng_ust_pytorch, op_exit, fn.name());
}

// Auto-register at library load (works under LD_PRELOAD, no python changes).
__attribute__((constructor)) static void tracer_pytorch_init() {
  at::addGlobalCallback(
      at::RecordFunctionCallback(&on_entry, &on_exit)
          .scopes({at::RecordScope::FUNCTION, at::RecordScope::BACKWARD_FUNCTION}));
}
