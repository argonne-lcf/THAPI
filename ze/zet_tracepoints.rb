require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_zet

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#define ZE_ENABLE_OCL_INTEROP 1
#include <CL/cl.h>
#include <zet_api.h>
#include <zet_ddi.h>
EOF

$zet_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

