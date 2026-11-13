require_relative 'opencl_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #define CL_TARGET_OPENCL_VERSION 300
  #define CL_USE_DEPRECATED_OPENCL_1_0_APIS
  #define CL_USE_DEPRECATED_OPENCL_1_1_APIS
  #define CL_USE_DEPRECATED_OPENCL_1_2_APIS
  #define CL_USE_DEPRECATED_OPENCL_2_0_APIS
  #define CL_USE_DEPRECATED_OPENCL_2_1_APIS
  #define CL_USE_DEPRECATED_OPENCL_2_2_APIS
  #include <CL/opencl.h>
  #include <CL/cl_egl.h>
  #include <dlfcn.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <string.h>
  #include <pthread.h>
  #include <sys/mman.h>
  #include <ffi.h>
  #include "uthash.h"
  #include "utlist.h"

  #include "opencl_tracepoints.h"
  #include "opencl_profiling.h"
  #include "opencl_source.h"
  #include "opencl_dump.h"
  #include "opencl_arguments.h"
  #include "opencl_build.h"
  #include "opencl_devices.h"

  #define CONCAT(c,suf) c ## _ ## suf
  #define STARTEV(command) CONCAT(command, #{START})
  #define STOPEV(command) CONCAT(command, #{STOP})
  #define do_tracepoint_safe(provider, name, ...) \
          do_tracepoint(provider, name, __VA_ARGS__)
  #define tracepoint_enabled_safe(provider, name) \
          tracepoint_enabled(provider, name)
  #define tracepoint_safe(provider, name, ...) \
          tracepoint(provider, name, __VA_ARGS__)
EOF

print_pointer_defines(CORE_COMMANDS, OPENCL_POINTER_NAMES)
print_pointer_table(CORE_COMMANDS, OPENCL_POINTER_NAMES)
print_ffi_wrapper_declarations(EXTENSION_COMMANDS)

# The libraries export the core functions, except the KHR and EXT ones:
# clGetExtensionFunctionAddress hands those out.
lookup = OPENCL_POINTER_NAMES[COMMANDS['clGetExtensionFunctionAddress']]
print_find_symbols('opencl', CORE_COMMANDS - LOOKED_UP_COMMANDS, OPENCL_POINTER_NAMES) do
  LOOKED_UP_COMMANDS.each do |c|
    puts <<EOF

  #{OPENCL_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)#{lookup}("#{c.name}");
  if (!#{OPENCL_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
  end
end

puts File.read(File.join(SRC_DIR, 'tracer_opencl_helpers.include.c'))

print_traced_wrappers(CORE_COMMANDS, :lttng_ust_opencl, OPENCL_POINTER_NAMES)
print_ffi_wrappers(EXTENSION_COMMANDS, :lttng_ust_opencl, OPENCL_POINTER_NAMES)
