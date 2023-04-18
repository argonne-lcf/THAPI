require_relative 'gen_hip_library_base.rb'
require_relative '../utils/gen_babeltrace_lib_helper'

puts <<EOF
require_relative 'hip_library.rb'
EOF

add_babeltrace_event_callbacks("hip_babeltrace_model.yaml")
