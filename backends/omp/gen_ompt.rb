require_relative 'ompt_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #include <stdint.h>
  #include <stddef.h>
  #include <omp-tools.h>
  #include "ompt_tracepoints.h"
EOF

normal_wrapper = lambda { |c, provider|
  print_wrapper(c, storage: 'static ') {
    print_tracepoint_locals(c)
    print_tracepoint_call(provider, c, nil, tracepoint_call_args(c))
  }
}

COMMANDS.each do |c|
  next if c.name == 'ompt_callback_control_tool_func'

  normal_wrapper.call(c, :lttng_ust_ompt)
end

puts File.read('tracer_ompt_helpers.include.c')
