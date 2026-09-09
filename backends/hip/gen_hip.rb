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

print_find_symbols('hip', COMMANDS, HIP_POINTER_NAMES)

puts File.read(File.join(SRC_DIR, 'tracer_hip_helpers.include.c'))

print_traced_wrappers(COMMANDS, :lttng_ust_hip, HIP_POINTER_NAMES)
