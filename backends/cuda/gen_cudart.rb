require_relative 'cudart_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #define _GNU_SOURCE
  #include <dlfcn.h>
  #define __CUDA_API_VERSION_INTERNAL 1
  #include <cuda_runtime_api.h>
  #include <pthread.h>
  #include "cudart_tracepoints.h"
EOF

print_pointer_defines(COMMANDS, CUDART_POINTER_NAMES)

print_pointer_table(COMMANDS, CUDART_POINTER_NAMES)

puts <<~EOF

  static void find_cudart_symbols(void * handle, int verbose) {
EOF

print_dlsym_lookups(COMMANDS, CUDART_POINTER_NAMES)

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_cudart_helpers.include.c'))

normal_wrapper = lambda { |c, provider|
  print_wrapper(c, init: '_init_tracer();') { print_traced_body(c, provider, CUDART_POINTER_NAMES) }
}

COMMANDS.each do |c|
  normal_wrapper.call(c, :lttng_ust_cudart)
end

COMMANDS.each do |c|
  puts "__asm__(\".symver #{c.name},#{c.name}@@libcudart.so.12, remove\");"
end
