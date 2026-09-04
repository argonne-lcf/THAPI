require 'set'
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

  print_enum_with_namespace(NAMING, e.name, e)
end

# hip typedefs structs its own headers only forward-declare; those carry no
# layout to emit.
print_typedefs(NAMING, struct: lambda { |name, t|
  struct = API.struct(t.type, opaque_ok: true)
  print_struct_prepending_uuid(NAMING, name, struct) if struct
})

puts <<~EOF
  end
EOF
