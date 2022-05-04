require_relative 'cuda_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_cuda

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include <cuda.h.include>
EOF

$cuda_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

