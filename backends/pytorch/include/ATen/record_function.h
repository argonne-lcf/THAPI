// Minimal hand-written stand-in for PyTorch's <ATen/record_function.h>.
//
// Declares only the names tracer_pytorch.cpp actually uses:
//   at::RecordScope             - FUNCTION / BACKWARD_FUNCTION values
//   at::ObserverContext         - empty base; we only ever return nullptr
//   at::RecordFunction          - only .name() and .overload_name(); we
//                                 never construct one ourselves, only
//                                 receive a reference from the real library,
//                                 so no field layout is needed here -- both
//                                 resolve against their real out-of-line
//                                 symbols in libtorch_cpu.
//   at::RecordFunctionCallback  - constructed BY US and passed BY VALUE into
//                                 addGlobalCallback. Its field layout below
//                                 (order, types, and the scopes_ array sized
//                                 by NUM_SCOPES) must match PyTorch's real
//                                 class byte-for-byte, copied verbatim from
//                                 the upstream header. If a future PyTorch
//                                 release reorders/adds a field or changes
//                                 RecordScope's member count, this header
//                                 will still compile cleanly but
//                                 addGlobalCallback will read the wrong
//                                 bytes back -- silent corruption, not a
//                                 build failure. Re-verify this layout
//                                 against ATen/record_function.h whenever
//                                 upstream PyTorch changes.
//   at::addGlobalCallback       - registers the callback pair
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_set>

namespace at {

enum class RecordScope : uint8_t {
  FUNCTION = 0,
  BACKWARD_FUNCTION,
  TORCHSCRIPT_FUNCTION,
  KERNEL_FUNCTION_DTYPE,
  CUSTOM_CLASS,
  BUILD_FEATURE,
  LITE_INTERPRETER,
  USER_SCOPE,
  STATIC_RUNTIME_OP,
  STATIC_RUNTIME_MODEL,
  NUM_SCOPES,
};

struct ObserverContext {
  virtual ~ObserverContext() = default;

protected:
  ObserverContext() = default;
};

struct RecordFunction {
  const char *name() const;
  const char *overload_name() const;
};

class RecordFunctionCallback {
public:
  using StartCallback = std::unique_ptr<ObserverContext> (*)(const RecordFunction &);
  using EndCallback = void (*)(const RecordFunction &, ObserverContext *);

  explicit RecordFunctionCallback(StartCallback start, EndCallback end = nullptr)
      : start_(start), end_(end) {
    scopes_.fill(true);
  }

  RecordFunctionCallback &scopes(const std::unordered_set<RecordScope> &scopes) {
    if (!scopes.empty()) {
      scopes_.fill(false);
      for (auto sc : scopes) {
        scopes_[static_cast<std::size_t>(sc)] = true;
      }
    } else {
      scopes_.fill(true);
    }
    return *this;
  }

private:
  StartCallback start_;
  EndCallback end_;
  double sampling_prob_ = 1.0;
  std::array<bool, static_cast<std::size_t>(RecordScope::NUM_SCOPES)> scopes_ = {};
  bool needs_inputs_ = false;
  bool needs_outputs_ = false;
  bool needs_ids_ = false;
};

using CallbackHandle = uint64_t;

CallbackHandle addGlobalCallback(RecordFunctionCallback cb);

} // namespace at
