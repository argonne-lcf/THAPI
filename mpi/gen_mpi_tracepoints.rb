require_relative 'mpi_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_mpi

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include <mpi.h.include>
EOF

$ompt_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c)
}
