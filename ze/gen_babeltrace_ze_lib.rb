require_relative 'gen_ze_library_base.rb'
require_relative '../utils/gen_babeltrace_lib_helper'

puts <<EOF
require_relative 'ze_library.rb'
EOF

add_babeltrace_event_callbacks("ze_babeltrace_model.yaml")
