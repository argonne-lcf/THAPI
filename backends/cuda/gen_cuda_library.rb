require_relative 'gen_cuda_library_base'

print_ffi_module(:CUDA)

puts <<~EOF
  module CUDA
    CU_COMPUTE_ACCELERATED_TARGET_BASE = 0x10000
    CU_TARGET_COMPUTE_90 = 90
    CU_TENSOR_MAP_NUM_QWORDS = 16
    CU_IPC_HANDLE_SIZE = 64
    CUDA_IPC_HANDLE_SIZE = 64
    extend FFI::Library

EOF

print_handle_uuid_modules
puts

puts <<EOF
  typedef :uint32, #{to_ffi_name('cuuint32_t')}
  typedef :uint64, #{to_ffi_name('cuuint64_t')}
  typedef :pointer, #{to_ffi_name('CUdeviceptr')}
  typedef :uint32, #{to_ffi_name('CUdeviceptr_v1')}
  typedef :uint64, #{to_ffi_name('CUtexObject')}
  typedef :uint64, #{to_ffi_name('CUsurfObject')}
  typedef :uint64, #{to_ffi_name('CUmemGenericAllocationHandle_v1')}
  typedef #{to_ffi_name('CUmemGenericAllocationHandle_v1')}, #{to_ffi_name('CUmemGenericAllocationHandle')}
EOF

# The plain pointer and integer typedefs cuda declares are spelled out by hand
# above, so the shared printer must not emit them a second time.
print_typedefs(
  NAMING,
  struct: ->(name, t) { print_struct_prepending_uuid(NAMING, name, API.struct(t.type)) },
  pointer: nil,
  integer: nil
)

puts <<~EOF
  end
EOF
