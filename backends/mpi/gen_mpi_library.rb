require_relative 'gen_mpi_library_base'

print_ffi_module(:MPI)

puts <<~EOF
  module MPI
    extend FFI::Library

EOF

print_handle_uuid_modules

API.types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = API.enum(t.type)
    print_enum_with_namespace(NAMING, t.name, enum)
  elsif API.object?(t.name)
    print_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = API.struct(t.type)
    print_struct_with_namespace(NAMING, t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = API.union(t.type)
    print_union_with_namespace(NAMING, t.name, union)
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
