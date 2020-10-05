require_relative 'cudart_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_cudart

puts <<EOF
#define __CUDA_API_VERSION_INTERNAL 1
#include <cuda_runtime_api.h>
EOF

$cudart_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

