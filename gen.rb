require_relative 'opencl_model'


provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
EOF

puts <<EOF
#include "opencl_tracepoints.h"
#include "opencl_profiling.h"
#include "opencl_source.h"
EOF

$opencl_commands.each { |c|
  puts <<EOF
static #{c.decl_pointer} = (void *) 0x0;
EOF
}

puts <<EOF
void CL_CALLBACK  event_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  if (tracepoint_enabled(#{provider}_profiling, event_profiling_results)) {
    cl_ulong queued;
    cl_ulong submit;
    cl_ulong start;
    cl_ulong end;
    cl_int queued_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, NULL);
    cl_int submit_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
    cl_int start_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    cl_int end_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling_results, event, event_command_exec_status,
              queued_status, queued, submit_status, submit, start_status, start, end_status, end);
  }
}

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
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts "  tracepoint(#{provider}, #{c.prototype.name}_start, #{(params+tracepoint_params).join(", ")});"
  c.preludes.each { |p|
    puts p
  }
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{c.prototype.pointer_name}(#{params.join(", ")});
EOF
  else
    puts "  #{c.prototype.pointer_name}(#{params.join(", ")});"
  end
  c.epilogues.each { |e|
    puts e
  }
  if c.prototype.has_return_type?
    params.push "_retval"
    puts <<EOF
  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});
  return _retval;
}

EOF
  else
    puts <<EOF
  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});
}

EOF
  end
}
