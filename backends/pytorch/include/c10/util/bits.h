#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <torch/headeronly/util/bits.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
