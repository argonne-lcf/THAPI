require_relative 'gen_cuda_library_base'

def print_enum(name, enum)
  print_enum_with_namespace(:CUDA, name, enum)
end

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

def print_struct(name, struct)
  prepends = []
  prepends << 'UUID' if to_class_name(name).match('UUID')
  print_struct_with_namespace(:CUDA, name, struct, prepends: prepends)
end

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

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  elsif $objects.include?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = $all_structs.find { |s| t.type.name == s.name }
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union_with_namespace(:CUDA, t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  end
end

puts <<~EOF
  end
EOF
