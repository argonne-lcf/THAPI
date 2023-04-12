require_relative 'hip_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_hip

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include <hip.h.include>
EOF

$hip_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

