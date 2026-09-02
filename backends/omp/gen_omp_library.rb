require_relative 'gen_omp_library_base'

def print_enum(name, enum)
  if enum.name.end_with?('flag_t')
    print_bitfield_with_namespace(NAMING, name, enum, check_flags: true)
  else
    print_enum_with_namespace(NAMING, name, enum, filter_members: ->(m) { m.name != 'ompt_callback_master' })
  end
end

print_ffi_module(:OMP, struct: false, union: false, inline_array: false)

puts <<~EOF

  module OMP
    extend FFI::Library

EOF

# The Ruby bindings for OMPT carry only its enums.
print_typedefs(NAMING,
               enum: ->(name, t) { print_enum(name, API.enum(t.type)) },
               object: nil, struct: nil, union: nil,
               function_pointer: nil, pointer: nil, integer: nil)

puts <<~EOF
  end
EOF
