#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <string>

#if !defined(FBCODE_CAFFE2) && !defined(C10_NO_DEPRECATED)

namespace c10 {

// NOLINTNEXTLINE(misc-unused-using-decls)
using std::stod;
// NOLINTNEXTLINE(misc-unused-using-decls)
using std::stoi;
// NOLINTNEXTLINE(misc-unused-using-decls)
using std::stoll;
// NOLINTNEXTLINE(misc-unused-using-decls)
using std::stoull;
// NOLINTNEXTLINE(misc-unused-using-decls)
using std::to_string;

} // namespace c10

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
