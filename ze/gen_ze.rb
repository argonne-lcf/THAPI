require_relative 'ze_model'

puts <<EOF
#define ZE_ENABLE_OCL_INTEROP 1
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include <stdint.h>
#include <stddef.h>
#include <ze_api.h>
#include <ze_ddi.h>
#include <zet_api.h>
#include <zet_ddi.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <ffi.h>
#include "uthash.h"

#include "ze_tracepoints.h"
#include "zet_tracepoints.h"
#include "ze_profiling.h"

EOF

($ze_commands + $zet_commands).each { |c|
  puts "#define #{ZE_POINTER_NAMES[c]} #{c.pointer_name}"
}

($ze_commands + $zet_commands).each { |c|
  puts <<EOF

#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{ZE_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

puts <<EOF

static void find_ze_symbols(void * handle) {
EOF

($ze_commands + $zet_commands).each { |c|
  puts <<EOF

  #{ZE_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{ZE_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
}

puts <<EOF
}

EOF

puts File::read("tracer_ze_helpers.include.c")

common_block = lambda { |c, provider|
  params = c.parameters.collect(&:name)
  tp_params = c.parameters.collect { |p|
    if p.type.kind_of?(YAMLCAst::Pointer) && p.type.type.kind_of?(YAMLCAst::Function)
      "(void *)(intptr_t)" + p.name
    else
      p.name
    end
  }
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_start, #{(tp_params+tracepoint_params).join(", ")});
EOF

  c.prologues.each { |p|
    puts p
  }

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = #{ZE_POINTER_NAMES[c]}(#{params.join(", ")});
EOF
  else
    puts "  #{ZE_POINTER_NAMES[c]}(#{params.join(", ")});"
  end
  c.epilogues.each { |e|
    puts e
  }
  if c.has_return_type?
    tp_params.push "_retval"
  end
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_stop, #{(tp_params+tracepoint_params).join(", ")});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<EOF
#{c.decl} {
EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
  common_block.call(c, provider)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<EOF
}

EOF
} 

$ze_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_ze)
}
$zet_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zet)
}
