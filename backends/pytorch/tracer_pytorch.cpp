#include <ATen/record_function.h>

#include "pytorch.h"
#include <string>

// PyTorch identifies an operator by TWO strings: a schema name (e.g.
// "aten::abs") and an overload name (e.g. "" for the default overload,
// "out" for the variant that writes into a caller-supplied output tensor).
// fn.name() alone returns only the schema name, so two different overloads
// of the same op are indistinguishable in the trace. This matters because
// PyTorch's own operators frequently call one overload from another: the
// default abs(Tensor) allocates an output tensor and then calls abs.out()
// to actually compute the result, so a trace keyed on fn.name() alone shows
// "aten::abs" entering, then "aten::abs" entering AGAIN before the first
// one exits -- indistinguishable from the op reentering itself, even though
// it is really two different overloads, one nested inside the other.
// Emitting fn.name() + "." + fn.overload_name() keeps the two distinguishable.
static std::string qualified_name(const at::RecordFunction &fn) {
  const char *overload = fn.overload_name();
  return (overload[0] == '\0') ? fn.name() : std::string(fn.name()) + "." + overload;
}

// ENTRY: fires BEFORE the op runs. LTTng adds time + vpid/vtid via context.
static std::unique_ptr<at::ObserverContext> on_entry(const at::RecordFunction &fn) {
  tracepoint(lttng_ust_pytorch, op_entry, qualified_name(fn).c_str());
  return nullptr;
}

// EXIT: fires AFTER the op returns.
static void on_exit(const at::RecordFunction &fn, at::ObserverContext *) {
  tracepoint(lttng_ust_pytorch, op_exit, qualified_name(fn).c_str());
}

// Auto-register at library load (works under LD_PRELOAD, no python changes).
__attribute__((constructor)) static void tracer_pytorch_init() {
  at::addGlobalCallback(
      at::RecordFunctionCallback(&on_entry, &on_exit)
          .scopes({at::RecordScope::FUNCTION, at::RecordScope::BACKWARD_FUNCTION}));
}
