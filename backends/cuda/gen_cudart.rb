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

print_find_symbols('cudart', COMMANDS, CUDART_POINTER_NAMES)

puts File.read(File.join(SRC_DIR, 'tracer_cudart_helpers.include.c'))

# cudart initializes from every wrapper: no function is singled out.
print_traced_wrappers(COMMANDS, :lttng_ust_cudart, CUDART_POINTER_NAMES,
                      init: ->(_c) { '_init_tracer();' })

COMMANDS.each do |c|
  puts "__asm__(\".symver #{c.name},#{c.name}@@libcudart.so.13, remove\");"
end
