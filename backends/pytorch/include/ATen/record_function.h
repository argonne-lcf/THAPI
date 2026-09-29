// Minimal hand-written PyTorch's <ATen/record_function.h>.

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
