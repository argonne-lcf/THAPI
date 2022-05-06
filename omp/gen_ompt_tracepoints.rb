require_relative 'ompt_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_ompt

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include <ompt.h.include>
EOF

$ompt_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c)
}
