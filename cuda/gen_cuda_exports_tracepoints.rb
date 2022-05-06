require_relative 'cuda_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_cuda_exports

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include <cuda.h.include>
EOF

h = YAML::load_file(File.join(SRC_DIR,"cuda_events.yaml"))[provider.to_s]
enums = h["enums"]

if enums
  enums.each { |e|
    LTTng.print_enum(provider, e)
  }
end

h["events"].each { |e|
  LTTng.print_tracepoint(provider, e)
}

$cuda_exports_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
}

