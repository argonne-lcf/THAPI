require_relative 'ompt_model'

puts <<~EOF
  #include <stdint.h>
  #include <stddef.h>
  #include <omp-tools.h>
  #include "ompt_tracepoints.h"
EOF

common_block = lambda { |c, provider|
  c.parameters.collect(&:name)
  tp_params = c.parameters.collect do |p|
    if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
      '(void *)(intptr_t)' + p.name
    else
      p.name
    end
  end
  c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init unless p.after?
  end
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts <<EOF
  tracepoint(#{provider}, #{c.name.gsub(/_func\z/, '')}, #{(tp_params + tracepoint_params).join(', ')});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<~EOF
    static #{c.decl} {
  EOF
  common_block.call(c, provider)
  puts <<~EOF
    }

  EOF
}

$ompt_commands.each do |c|
  next if c.name == 'ompt_callback_control_tool_func'

  normal_wrapper.call(c, :lttng_ust_ompt)
end

puts File.read('tracer_ompt_helpers.include.c')
