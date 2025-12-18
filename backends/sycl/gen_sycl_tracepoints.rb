require_relative 'sycl_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_sycl

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include "sycl-tools.h"
EOF

$sycl_commands.each { |c|
  $tracepoint_lambda.call(provider, c)
}
