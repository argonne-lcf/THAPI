require_relative 'opencl_model'

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
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
EOF

$opencl_commands.each { |c|
  puts <<EOF

typedef #{c.decl_pointer(type: true)};
static #{c.prototype.pointer_type_name}  #{c.prototype.pointer_name} = (void *) 0x0;
EOF
}

$opencl_extension_commands.each { |c|
  puts <<EOF

typedef #{c.decl_pointer(type: true)};
#{c.decl_ffi_wrapper};
EOF
}

puts <<EOF

static void find_opencl_symbols(void * handle) {
EOF

$opencl_commands.each { |c|
  unless (c.extension? && c.prototype.name.match(/KHR$|EXT$/))
    puts <<EOF

  #{c.prototype.pointer_name} = (#{c.prototype.pointer_type_name})(intptr_t)dlsym(handle, "#{c.prototype.name}") ;
  if (!#{c.prototype.pointer_name})
    fprintf(stderr, "Missing symbol #{c.prototype.name}!\\n");
EOF
  end
}

$opencl_commands.each { |c|
  if (c.extension? && c.prototype.name.match(/KHR$|EXT$/))
    puts <<EOF

  #{c.prototype.pointer_name} = (#{c.prototype.pointer_type_name})(intptr_t)#{$clGetExtensionFunctionAddress.prototype.pointer_name}("#{c.prototype.name}");
  if (!#{c.prototype.pointer_name})
    fprintf(stderr, "Missing symbol #{c.prototype.name}!\\n");
EOF
  end
}

puts <<EOF
}

#define CL_GET_EVENT_PROFILING_INFO_PTR #{$clGetEventProfilingInfo.prototype.pointer_name}
#define CL_GET_KERNEL_INFO_PTR #{$clGetKernelInfo.prototype.pointer_name}
#define CL_ENQUEUE_READ_BUFFER_PTR #{$clEnqueueReadBuffer.prototype.pointer_name}
#define CL_SET_EVENT_CALLBACK_PTR #{$clSetEventCallback.prototype.pointer_name}
#define CL_RELEASE_EVENT_PTR #{$clReleaseEvent.prototype.pointer_name}
#define CL_ENQUEUE_SVM_MEMCPY_PTR #{$clEnqueueSVMMemcpy.prototype.pointer_name}
#define CL_EMQUEUE_BARRIER_WITH_WAIT_LIST_PTR #{$clEnqueueBarrierWithWaitList.prototype.pointer_name}
#define CL_GET_DEVICE_INFO_PTR #{$clGetDeviceInfo.prototype.pointer_name}
#define CL_GET_PROGRAM_INFO_PTR #{$clGetProgramInfo.prototype.pointer_name}
#define CL_GET_PROGRAM_BUILD_INFO_PTR #{$clGetProgramBuildInfo.prototype.pointer_name}
#define CL_GET_PLATFORM_INFO_PTR #{$clGetPlatformInfo.prototype.pointer_name}
#define CL_GET_KERNEL_INFO_PTR #{$clGetKernelInfo.prototype.pointer_name}
#define CL_GET_KERNEL_ARG_INFO_PTR #{$clGetKernelArgInfo.prototype.pointer_name}

EOF

puts File::read("tracer_opencl_helpers.include.c")

common_block = lambda { |c|
  params = []
  tp_params = []
  unless c.void_parameters?
    params = c.parameters.collect(&:name)
    tp_params = c.parameters.collect { |p| (p.callback? ? "(void *)(intptr_t)" : "" ) + p.name }
  end
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts <<EOF
  tracepoint(lttng_ust_opencl, #{c.prototype.name}_start, #{(tp_params+tracepoint_params).join(", ")});
EOF
  c.prologues.each { |p|
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
    tp_params.push "_retval"
  end
  puts <<EOF
  tracepoint(lttng_ust_opencl, #{c.prototype.name}_stop, #{(tp_params+tracepoint_params).join(", ")});
EOF
}

$opencl_commands.each { |c|
  puts <<EOF
#{c.decl} {
EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
  common_block.call(c)
  if c.prototype.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<EOF
}

EOF
}

$opencl_extension_commands.each { |c|
  puts <<EOF
#{c.decl_ffi_wrapper} {
  (void)cif;
EOF
  c.parameters.each_with_index { |p, i|
    puts <<EOF
  #{p.decl} = *(#{p.decl_pointer} *)args[#{i}];
EOF
  }
  common_block.call(c)
  if c.prototype.has_return_type?
    puts <<EOF
  *ffi_ret = _retval;
EOF
  end
  puts <<EOF
}
EOF
}
