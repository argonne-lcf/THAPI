require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_cuda_exports

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include <cuda.h.include>
EOF

h = YAML.load_file(File.join(SRC_DIR, 'cuda_events.yaml'))[provider.to_s]
enums = h['enums']

if enums
  enums.each do |e|
    LTTng.print_enum(provider, e)
  end
end

h['events'].each do |e|
  LTTng.print_tracepoint(provider, e)
end

$cuda_exports_commands.each do |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

  $tracepoint_lambda.call(provider, c, :start)
  $tracepoint_lambda.call(provider, c, :stop)
end
