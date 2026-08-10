require_relative 'gen_hip_library_base'

print_ffi_module(:HIP)

puts <<~EOF
  module HIP
    HIP_IPC_HANDLE_SIZE = 64
    extend FFI::Library

EOF

print_handle_uuid_modules

puts <<EOF

  hipDeviceAttributeCudaCompatibleBegin = 0
  hipDeviceAttributeAmdSpecificBegin = 10000
  HIPRTC_JIT_NUM_LEGACY_INPUT_TYPES = 6
  ACTIVITY_DOMAIN_HIP_OPS = 2
EOF

def print_struct(name, struct)
  prepends = []
  prepends << 'UUID' if to_class_name(name).match('UUID')
  print_struct_with_namespace(:HIP, name, struct, prepends: prepends)
end

typedef_enum_names = $all_types.filter_map { |t| t.type.name if t.type.is_a?(YAMLCAst::Enum) }.to_set
$all_enums.each do |e|
  next unless e.name
  next if typedef_enum_names.include?(e.name)

  print_enum_with_namespace(:HIP, e.name, e)
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum_with_namespace(:HIP, t.name, enum)
  elsif $objects.include?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = t.type.name ? $all_structs.find { |s| t.type.name == s.name } : t.type
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union_with_namespace(:HIP, t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  elsif t.type.is_a?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.is_a?(YAMLCAst::Int)
    print_int_type(t.name, t.type.name)
  else
    # $stderr.puts t.inspect
  end
end

puts <<~EOF
  end
EOF
