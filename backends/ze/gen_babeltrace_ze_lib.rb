require_relative 'gen_ze_library_base'
require_relative '../../utils/gen_babeltrace_lib_helper'

puts <<~EOF
  require_relative 'ze_library.rb'
EOF

add_babeltrace_event_callbacks('btx_ze_model.yaml')
