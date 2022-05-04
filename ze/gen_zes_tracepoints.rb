require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_zes

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include "ze.h.include"
EOF

$zes_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

