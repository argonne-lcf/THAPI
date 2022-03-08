require_relative 'gen_cuda_library_base.rb'
require_relative '../utils/gen_babeltrace_lib_helper'

puts <<EOF
require_relative 'cuda_library.rb'
EOF

add_babeltrace_event_callbacks("cuda_babeltrace_model.yaml")
