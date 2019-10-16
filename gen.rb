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
  params = []
  params = c.parameters.collect(&:name) unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
  if INSTR == :printf
    puts '  printf("Called: #{c.prototype.name}\\n");'
  elsif INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
    tracepoint_params = c.tracepoint_parameters.collect(&:name)
    c.tracepoint_parameters.each { |p|
      puts "  #{p.type} #{p.name};"
    }
    c.tracepoint_parameters.each { |p|
      puts p.init
    }
    puts "  tracepoint(lttng_ust_opencl, #{c.prototype.name}_start, #{(params+tracepoint_params).join(", ")});"
  else
    $stderr.puts "Skipped: #{c.prototype.name}"
  end
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{c.prototype.pointer_name}(#{params.join(", ")});
EOF
    if INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
      params.push "_retval"
      puts "  tracepoint(lttng_ust_opencl, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});"
    end
    puts <<EOF
  return _retval;
}
EOF
  else
    puts "  #{c.prototype.pointer_name}(#{params.join(", ")});"
    if INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
      puts "  tracepoint(lttng_ust_opencl, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});"
    end
    puts "}"
  end
}
