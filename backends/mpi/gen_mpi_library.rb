require_relative 'gen_mpi_library_base'

print_ffi_module(:MPI)

puts <<~EOF
  module MPI
    extend FFI::Library

EOF

print_handle_uuid_modules

print_typedefs(NAMING)

puts <<~EOF
  end
EOF
