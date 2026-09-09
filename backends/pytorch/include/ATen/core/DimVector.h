#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/util/DimVector.h>

namespace at {

// Redeclaring 'DimVector' type and size inside 'at' namespace.
// This is done to avoid modifying every use into their 'c10'
// equivalent.

using c10::kDimVectorStaticSize;
using c10::DimVector;

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
