require_relative 'ompt_model'

puts <<EOF
#include <stdint.h>
#include <stddef.h>
#include <omp-tools.h>
#include "ompt_tracepoints.h"
EOF


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
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts <<EOF
  tracepoint(#{provider}, #{c.name.gsub(/_func\z/,'')}, #{(tp_params+tracepoint_params).join(", ")});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<EOF
static #{c.decl} {
EOF
  common_block.call(c, provider)
  puts <<EOF
}

EOF
}

$ompt_commands.each { |c|
  next if c.name == "ompt_callback_control_tool_func"
  normal_wrapper.call(c, :lttng_ust_ompt)
}

puts File::read("tracer_ompt_helpers.include.c")
