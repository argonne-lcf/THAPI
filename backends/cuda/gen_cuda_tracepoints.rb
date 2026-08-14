require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_cuda

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include <cuda.h.include>
EOF

COMMANDS.groups[provider].each do |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

  print_tracepoint(provider, c, :start)
  print_tracepoint(provider, c, :stop)
end
