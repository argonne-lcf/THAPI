require_relative 'cuda_model'

puts <<EOF
#define __CUDA_API_VERSION_INTERNAL 1
#include <cuda.h>
#include <pthread.h>
#include "cuda_tracepoints.h"
EOF

$cuda_commands.each { |c|
  puts "#define #{CUDA_POINTER_NAMES[c]} #{c.pointer_name}"
}

$cuda_commands.each { |c|
  puts <<EOF

#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{CUDA_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

puts <<EOF

static void find_cuda_symbols(void * handle) {
EOF

$cuda_commands.each { |c|
  puts <<EOF

  #{CUDA_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{CUDA_POINTER_NAMES[c]})
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
}

puts <<EOF
}

EOF

puts File::read(File.join(SRC_DIR,"tracer_cuda_helpers.include.c"))

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
    puts p.init unless p.after?
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
  _retval = #{CUDA_POINTER_NAMES[c]}(#{params.join(", ")});
EOF
  else
    puts "  #{CUDA_POINTER_NAMES[c]}(#{params.join(", ")});"
  end
  c.tracepoint_parameters.each { |p|
    puts p.init if p.after?
  }
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

$cuda_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_cuda)
}
