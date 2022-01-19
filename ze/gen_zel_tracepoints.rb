require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_zel

puts <<EOF
#include "ze.h.include"
EOF

$zel_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

