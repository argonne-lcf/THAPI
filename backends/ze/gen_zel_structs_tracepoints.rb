require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_zel_structs

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

tagged_structs(APIS[:zel]).each do |t|
  print_struct_tracepoint(provider, t)
end
