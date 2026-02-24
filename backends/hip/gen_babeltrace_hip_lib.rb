require_relative 'gen_hip_library_base'
require_relative '../../utils/gen_babeltrace_lib_helper'

puts <<~EOF
  require_relative 'hip_library.rb'
EOF

add_babeltrace_event_callbacks('btx_hip_model.yaml')
