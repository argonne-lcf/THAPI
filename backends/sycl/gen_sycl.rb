require_relative 'sycl_model'

# Customization of codegen

# Printing
common_block = lambda { |c, provider|
  params = c.parameters.collect(&:name)
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  l = ["tracepoint(#{provider}, #{c.name}, #{(params+tracepoint_params).join(", ")});"]
  "  "+l.join("\n  ")
}

puts <<EOF
#include "sycl_tracepoints.h"
EOF

provider = :lttng_ust_sycl
puts $sycl_commands.filter_map { |c|
  next if c.function.inline
  l  = ["static void _#{c.name}(void *state)  {"]
  l += [common_block.call(c, provider)]
  l += ["}"]
}.join("  \n")

puts <<EOF
void init_register() {
EOF

puts $sycl_commands.filter_map { |c|
  "  #{c.name}(_#{c.name});"
}.join("  \n")

puts <<EOF
}
EOF

