require_relative 'gen_omp_library_base.rb'
require_relative '../utils/gen_babeltrace_lib_helper'

puts <<EOF
require_relative 'omp_library.rb'
EOF

add_babeltrace_event_callbacks("omp_babeltrace_model.yaml")
