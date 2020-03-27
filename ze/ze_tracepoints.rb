require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_ze

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#define ZE_ENABLE_OCL_INTEROP 1
#include <ze_api.h>
#include <ze_ddi.h>
EOF

$ze_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

