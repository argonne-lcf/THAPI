require_relative 'gen_omp_library_base'

def print_bitfield(name, enum)
  print_bitfield_with_namespace(:OMP, name, enum, check_flags: true)
end

def print_enum(name, enum)
  if enum.name.end_with?('flag_t')
    print_bitfield_with_namespace(:OMP, name, enum, check_flags: true)
  else
    print_enum_with_namespace(:OMP, name, enum, filter_members: ->(m) { m.name != 'ompt_callback_master' })
  end
end

print_ffi_module(:OMP, struct: false, union: false, inline_array: false)

puts <<~EOF

  module OMP
    extend FFI::Library

EOF

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  end
end

puts <<~EOF
  end
EOF
