#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/core/ScalarType.h>
#include <c10/macros/Export.h>

namespace caffe2 {
class TypeMeta;
} // namespace caffe2

namespace c10 {
C10_API void set_default_dtype(caffe2::TypeMeta dtype);
C10_API const caffe2::TypeMeta get_default_dtype();
C10_API ScalarType get_default_dtype_as_scalartype();
C10_API const caffe2::TypeMeta get_default_complex_dtype();
} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
