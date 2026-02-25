require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_lib_helper'

puts <<~EOF
  require_relative 'itt_library.rb'
EOF

add_babeltrace_event_callbacks('btx_itt_model.yaml')
