#include <ATen/record_function.h>
#include "pytorch_tracepoints.h"

static std::unique_ptr<at::ObserverContext> on_entry(const at::RecordFunction &fn) {
  tracepoint(lttng_ust_pytorch, op_entry, fn.name(), fn.overload_name());
  return nullptr;
}

static void on_exit(const at::RecordFunction &fn, at::ObserverContext *) {
  tracepoint(lttng_ust_pytorch, op_exit, fn.name(), fn.overload_name());
}

__attribute__((constructor)) static void tracer_pytorch_init() {
  at::addGlobalCallback(
      at::RecordFunctionCallback(&on_entry, &on_exit)
          .scopes({at::RecordScope::FUNCTION, at::RecordScope::BACKWARD_FUNCTION}));
}
