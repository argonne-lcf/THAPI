require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_ompt

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include <ompt.h.include>
EOF

COMMANDS.groups[provider].each do |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

  print_tracepoint(provider, c)
end
