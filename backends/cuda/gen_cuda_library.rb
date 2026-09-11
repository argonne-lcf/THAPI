require_relative 'gen_cuda_library_base'

# cuda 13 spells some flag values `1u << n`, which Ruby cannot read, and
# declares a coredump flag the bindings must not carry.
def print_enum(naming, name, enum)
  print_enum_with_namespace(
    naming, name, enum,
    filter_members: ->(m) { m.name != 'CU_COREDUMP_LIGHTWEIGHT_FLAGS' },
    fix_values: ->(v) { v.gsub('1u <<', '1 <<') }
  )
end

print_ffi_module(NAMING)

puts <<~EOF
  module CUDA
    CU_COMPUTE_ACCELERATED_TARGET_BASE = 0x10000
    CU_COMPUTE_FAMILY_TARGET_BASE = 0x20000
    CU_TARGET_COMPUTE_90 = 90
    CU_TARGET_COMPUTE_100 = 100
    CU_TARGET_COMPUTE_103 = 103
    CU_TARGET_COMPUTE_110 = 110
    CU_TARGET_COMPUTE_120 = 120
    CU_TARGET_COMPUTE_121 = 121
    CU_TENSOR_MAP_NUM_QWORDS = 16
    CU_IPC_HANDLE_SIZE = 64
    CUDA_IPC_HANDLE_SIZE = 64
    RESOURCE_ABI_EXTERNAL_BYTES = 48
    extend FFI::Library

EOF

print_rendering_module(NAMING, META_PARAMETERS_STRUCT)

puts <<EOF
  typedef :uint32, #{to_ffi_name('cuuint32_t')}
  typedef :uint64, #{to_ffi_name('cuuint64_t')}
  typedef :pointer, #{to_ffi_name('CUdeviceptr')}
  typedef :uint32, #{to_ffi_name('CUdeviceptr_v1')}
  typedef :uint64, #{to_ffi_name('CUtexObject')}
  typedef :uint64, #{to_ffi_name('CUsurfObject')}
  typedef :uint64, #{to_ffi_name('CUmemGenericAllocationHandle_v1')}
  typedef #{to_ffi_name('CUmemGenericAllocationHandle_v1')}, #{to_ffi_name('CUmemGenericAllocationHandle')}
  typedef :uint64, #{to_ffi_name('CUgraphConditionalHandle')}
EOF

# The plain pointer and integer typedefs cuda declares are spelled out by hand
# above, so the shared printer must not emit them a second time.
print_typedefs(
  NAMING,
  enum: ->(name, t) { print_enum(NAMING, name, API.enum(t.type, opaque_ok: true) || t.type) },
  struct: ->(name, t) { print_struct_rendered(NAMING, name, API.struct(t.type), META_PARAMETERS_STRUCT) },
  pointer: nil,
  integer: nil
)

puts <<~EOF
  end
EOF
