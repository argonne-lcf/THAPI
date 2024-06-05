require_relative 'mpi_model'

puts <<EOF
#include <stdint.h>
#include <stddef.h>
#include <mpi.h>
#include "mpi_tracepoints.h"
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
  normal_wrapper.call(c, :lttng_ust_mpi)
}

puts File::read("tracer_mpi_helpers.include.c")
