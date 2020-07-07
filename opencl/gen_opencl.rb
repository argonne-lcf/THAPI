require_relative 'opencl_model'

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
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
#include "opencl_devices.h"

EOF

$opencl_commands.each { |c|
  puts "#define #{OPENCL_POINTER_NAMES[c]} #{c.prototype.pointer_name}"
}

$opencl_commands.each { |c|
  puts <<EOF

typedef #{c.decl_pointer(type: true)};
static #{c.prototype.pointer_type_name}  #{OPENCL_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

$opencl_extension_commands.each { |c|
  puts <<EOF

typedef #{c.decl_pointer(type: true)};
static #{c.decl_ffi_wrapper};
EOF
}

puts <<EOF

static void find_opencl_symbols(void * handle) {
EOF

$opencl_commands.each { |c|
  unless (c.extension? && c.prototype.name.match(/KHR$|EXT$/))
    puts <<EOF

  #{OPENCL_POINTER_NAMES[c]} = (#{c.prototype.pointer_type_name})(intptr_t)dlsym(handle, "#{c.prototype.name}") ;
  if (!#{OPENCL_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.prototype.name}!\\n");
EOF
  end
}

$opencl_commands.each { |c|
  if (c.extension? && c.prototype.name.match(/KHR$|EXT$/))
    puts <<EOF

  #{OPENCL_POINTER_NAMES[c]} = (#{c.prototype.pointer_type_name})(intptr_t)#{OPENCL_POINTER_NAMES[$clGetExtensionFunctionAddress]}("#{c.prototype.name}");
  if (!#{OPENCL_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.prototype.name}!\\n");
EOF
  end
}

puts <<EOF
}

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
  if HOST_PROFILE
    puts <<EOF
  uint64_t _start_ts = 0;
  uint64_t _stop_ts = 0;
  uint64_t _duration = 0;
  if (do_host_profile)
    _start_ts = get_timestamp_ns();
EOF
  end
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{OPENCL_POINTER_NAMES[c]}(#{params.join(", ")});
EOF
  else
    puts "  #{OPENCL_POINTER_NAMES[c]}(#{params.join(", ")});"
  end
  if HOST_PROFILE
    puts <<EOF
  if (do_host_profile) {
    _stop_ts = get_timestamp_ns();
    _duration = _stop_ts - _start_ts;
  }
EOF
  end
  c.epilogues.each { |e|
    puts e
  }
  if c.prototype.has_return_type?
    tp_params.push "_retval"
  end
  if HOST_PROFILE
    tp_params.push "_duration"
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
static #{c.decl_ffi_wrapper} {
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
