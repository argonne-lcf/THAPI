#pragma once

enum backend_e {
  BACKEND_UNKNOWN = 0,
  BACKEND_ZE = 1,
  BACKEND_OPENCL = 2,
  BACKEND_CUDA = 3,
  BACKEND_OMP_TARGET_OPERATIONS = 4,
  BACKEND_OMP = 5,
  BACKEND_HIP = 6,
};
typedef enum backend_e backend_t;
typedef unsigned backend_level_t;
