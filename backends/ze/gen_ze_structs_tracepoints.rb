require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_ze_structs

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

tagged_structs(APIS[:ze]).each do |t|
  print_struct_tracepoint(provider, t)
end
