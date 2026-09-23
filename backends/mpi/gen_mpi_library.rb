require_relative 'gen_mpi_library_base'

print_ffi_module(NAMING)

puts <<~EOF
  module MPI
    extend FFI::Library

EOF

print_bytes_module(NAMING, META_PARAMETERS)

print_typedefs(NAMING)

puts <<~EOF
  end
EOF
