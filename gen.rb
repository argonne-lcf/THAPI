require_relative 'opencl_model'

INSTR = :lttng

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <dlfcn.h>
#include <stdio.h>
EOF

if INSTR == :lttng
  puts '#include "opencl_tracepoints.h"'
end

$opencl_commands.each { |c|
  puts <<EOF
static #{c.decl_pointer} = (void *) 0x0;
EOF
}

puts <<EOF
static void * handle;
__thread char buff_events[4096];
void __load_tracer(void) __attribute__((constructor));
void __load_tracer(void) {
  void * handle = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    printf("Failure: could not load OpenCL library!\\n");
    exit(1);
  }
EOF

$opencl_commands.each { |c|
  puts <<EOF
  #{c.prototype.pointer_name} = dlsym(handle, "#{c.prototype.name}") ;
EOF
}

puts <<EOF
}
EOF

$opencl_commands.each { |c|
  puts <<EOF
#{c.decl} {
EOF
  if INSTR == :printf
    puts '  printf("Called: #{c.prototype.name}\\n");'
  elsif INSTR == :lttng && c.parameters.length <= 10 
    puts "  tracepoint(lttng_ust_opencl, #{c.prototype.name}_start, #{c.parameters.collect(&:name).join(", ")});"
  end
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{c.prototype.pointer_name}(#{c.parameters.collect(&:name).join(", ")});
  return _retval;
}
EOF
  else
    puts <<EOF
  #{c.prototype.pointer_name}(#{c.parameters.collect(&:name).join(", ")});
}
EOF
  end
}
