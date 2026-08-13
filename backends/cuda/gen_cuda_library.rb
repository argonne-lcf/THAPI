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

API.types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = API.enum(t.type)
    print_enum_with_namespace(:CUDA, t.name, enum)
  elsif API.object?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = API.struct(t.type)
    print_struct_prepending_uuid(:CUDA, t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = API.union(t.type)
    print_union_with_namespace(:CUDA, t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  end
end

puts <<~EOF
  end
EOF
