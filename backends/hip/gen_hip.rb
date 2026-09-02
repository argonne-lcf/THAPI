require_relative 'hip_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #include <pthread.h>
  #include <sys/mman.h>
  #include <string.h>
  #include "hip_tracepoints.h"
EOF

print_pointer_defines(COMMANDS, HIP_POINTER_NAMES)

print_pointer_table(COMMANDS, HIP_POINTER_NAMES)

puts <<~EOF

  static void find_hip_symbols(void * handle, int verbose) {
EOF

print_dlsym_lookups(COMMANDS, HIP_POINTER_NAMES, indent: "\t")

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_hip_helpers.include.c'))

normal_wrapper = lambda { |c, provider|
  print_wrapper(c, init: ('_init_tracer();' if c.init?)) { print_traced_body(c, provider, HIP_POINTER_NAMES) }
}

COMMANDS.each do |c|
  normal_wrapper.call(c, :lttng_ust_hip)
end
