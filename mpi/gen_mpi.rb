require_relative 'mpi_model'

puts <<~EOF
  #include <stdint.h>
  #include <mpi.h>
  #include "mpi_tracepoints.h"
EOF

def common_block(c, provider)
  params = c.parameters.collect(&:name)
  tp_params = c.parameters.collect do |p|
    if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
      '(void *)(intptr_t)' + p.name
    else
      p.name
    end
  end
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init unless p.after?
  end
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  tracepoint_params = c.tracepoint_parameters.collect(&:name)

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = P#{c.name}(#{params.join(', ')});
EOF
  else
    puts "  P#{c.name}(#{params.join(', ')});"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init if p.after?
  end
  tp_params.push '_retval' if c.has_return_type?
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  puts '  return _retval;' if c.has_return_type?
end

def normal_wrapper(c, provider)
  puts <<~EOF
    #{c.decl} {
  EOF
  common_block(c, provider)
  puts <<~EOF
    }

  EOF
end

$mpi_commands.each do |c|
  next if c.name.start_with?("PMPI")
  normal_wrapper(c, :lttng_ust_mpi)
end
