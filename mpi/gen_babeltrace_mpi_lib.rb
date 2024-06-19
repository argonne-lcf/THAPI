require_relative 'gen_mpi_library_base.rb'
require_relative '../utils/gen_babeltrace_lib_helper'

puts <<EOF
require_relative 'mpi_library.rb'
EOF

add_babeltrace_event_callbacks("mpi_babeltrace_model.yaml")
