require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_zer

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

$zer_commands.each do |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
end
