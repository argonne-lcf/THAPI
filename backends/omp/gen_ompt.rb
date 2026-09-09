require_relative 'ompt_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #include <stdint.h>
  #include <stddef.h>
  #include <omp-tools.h>
  #include "ompt_tracepoints.h"
EOF

COMMANDS.each do |c|
  next if c.name == 'ompt_callback_control_tool_func'

  print_wrapper(c, storage: 'static ') { print_callback_body(c, :lttng_ust_ompt) }
end

puts File.read('tracer_ompt_helpers.include.c')
