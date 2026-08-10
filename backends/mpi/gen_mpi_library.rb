require_relative 'gen_mpi_library_base'

def print_enum(name, enum)
  print_enum_with_namespace(:MPI, name, enum)
end

print_ffi_module(:MPI)

puts <<~EOF
  module MPI
    extend FFI::Library

EOF

print_handle_uuid_modules

def print_union(name, union)
  print_union_with_namespace(:MPI, name, union)
end

def print_struct(name, struct)
  print_struct_with_namespace(:MPI, name, struct)
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  elsif $objects.include?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    # An anonymous target carries the layout itself, so the typedef is the
    # definition -- same guard hip already uses. Without it MPI_Status and
    # MPI_F08_status are silently dropped from the bindings.
    struct = t.type.name ? $all_structs.find { |s| t.type.name == s.name } : t.type
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union(t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  elsif t.type.is_a?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.is_a?(YAMLCAst::Int)
    print_int_type(t.name, t.type.name)
  end
end

puts <<~EOF
  end
EOF
