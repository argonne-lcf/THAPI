#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/core/SymInt.h>
#include <c10/core/impl/SizesAndStrides.h>
#include <c10/util/SmallVector.h>
#include <cstddef>
#include <cstdint>

namespace c10 {

constexpr size_t kDimVectorStaticSize = C10_SIZES_AND_STRIDES_MAX_INLINE_SIZE;

/// A container for sizes or strides
using DimVector = SmallVector<int64_t, kDimVectorStaticSize>;
using SymDimVector = SmallVector<c10::SymInt, kDimVectorStaticSize>;

} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
