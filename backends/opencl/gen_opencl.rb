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

# The libffi trampoline an extension function is wrapped in.
def decl_ffi_wrapper(c)
  "void #{c.ffi_name}(ffi_cif *cif, #{c.declaration('return_type')} *ffi_ret, void** args, #{c.pointer_type_name} #{c.pointer_name})"
end

print_pointer_defines(CORE_COMMANDS, OPENCL_POINTER_NAMES)

CORE_COMMANDS.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name}  #{OPENCL_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

EXTENSION_COMMANDS.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{decl_ffi_wrapper(c)};
  EOF
end

puts <<~EOF

  static void find_opencl_symbols(void * handle, int verbose) {
EOF

(CORE_COMMANDS - LOOKED_UP_COMMANDS).each do |c|
  puts <<EOF

  #{OPENCL_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}") ;
  if (!#{OPENCL_POINTER_NAMES[c]} && verbose)
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
end

LOOKED_UP_COMMANDS.each do |c|
  puts <<EOF

  #{OPENCL_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)#{OPENCL_POINTER_NAMES[COMMANDS['clGetExtensionFunctionAddress']]}("#{c.name}");
  if (!#{OPENCL_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
end

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_opencl_helpers.include.c'))

print_traced_wrappers(CORE_COMMANDS, :lttng_ust_opencl, OPENCL_POINTER_NAMES)

EXTENSION_COMMANDS.each do |c|
  puts <<~EOF
    static #{decl_ffi_wrapper(c)} {
      (void)cif;
  EOF
  c.declaration('params').each_with_index do |p, i|
    puts "  #{p['decl']} = *(#{p['type']} *)args[#{i}];"
  end
  print_traced_body(c, :lttng_ust_opencl, OPENCL_POINTER_NAMES)
  puts '  *ffi_ret = _retval;' if c.has_return_type?
  puts '}'
  puts
end
