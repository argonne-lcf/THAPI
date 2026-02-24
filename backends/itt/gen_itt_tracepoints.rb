require_relative 'itt_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_itt

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ittnotify.h"
EOF

$itt_commands.each do |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

  $tracepoint_lambda.call(provider, c)
end
