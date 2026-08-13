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

typedef_enum_names = API.types.filter_map { |t| t.type.name if t.type.is_a?(YAMLCAst::Enum) }.to_set
API.enums.each do |e|
  next unless e.name
  next if typedef_enum_names.include?(e.name)

  print_enum_with_namespace(:HIP, e.name, e)
end

API.types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = API.enum(t.type.name)
    print_enum_with_namespace(:HIP, t.name, enum)
  elsif API.object?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = t.type.name ? API.struct(t.type.name) : t.type
    next unless struct

    print_struct_prepending_uuid(:HIP, t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = API.union(t.type.name)
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
