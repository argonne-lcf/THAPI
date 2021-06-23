require_relative 'ze_model'

puts <<EOF
#include <stdint.h>
#include <stddef.h>
#include <ze_api.h>
#include <ze_ddi.h>
#include <zet_api.h>
#include <zet_ddi.h>
#include <zes_api.h>
#include <zes_ddi.h>
#include <layers/zel_tracing_api.h>
#include <layers/zel_tracing_ddi.h>
#include <layers/zel_tracing_register_cb.h>
#include <loader/ze_loader.h>
#define HMODULE void *
#include <loader/ze_loader_api.h>
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
#include "zes_tracepoints.h"
#include "zel_tracepoints.h"
#include "ze_profiling.h"
#include "ze_properties.h"

EOF
all_commands = $ze_commands + $zet_commands + $zes_commands + $zel_commands
(all_commands).each { |c|
  puts "#define #{ZE_POINTER_NAMES[c]} #{c.pointer_name}"
}

(all_commands).each { |c|
  puts <<EOF

#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{ZE_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

puts <<EOF

static void find_ze_symbols(void * handle) {
EOF

(all_commands).each { |c|
  puts <<EOF

  #{ZE_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{ZE_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
}

puts <<EOF
}

EOF

puts File::read(File.join(SRC_DIR,"tracer_ze_helpers.include.c"))

common_block = lambda { |c, provider|
  params = c.parameters ? c.parameters.collect(&:name) : []
  tp_params = if c.parameters
    c.parameters.collect { |p|
      if p.type.kind_of?(YAMLCAst::Pointer) && p.type.type.kind_of?(YAMLCAst::Function)
        "(void *)(intptr_t)" + p.name
      else
        p.name
      end
    }
  else
    []
  end
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params+tracepoint_params).join(", ")});
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
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params+tracepoint_params).join(", ")});
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
$zes_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zes)
}
$zel_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zel)
}
