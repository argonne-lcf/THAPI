require_relative 'mpi_model'

def common_block(c, provider)
  tp_params = c.parameters.collect do |p|
    if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
      '(void *)(intptr_t)' + p.name
    else
      p.name
    end
  end
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init unless p.after?
  end
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts "  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params + tracepoint_params).join(', ')});"

  params = c.parameters.collect(&:name)
  if c.has_return_type?
    puts "  #{c.type} _retval;"
    puts "  _retval = #{MPI_POINTER_NAMES[c]}(#{params.join(', ')});"
  else
    puts "  #{MPI_POINTER_NAMES[c]}(#{params.join(', ')});"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init if p.after?
  end

  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  tp_params.push '_retval' if c.has_return_type?
  puts "  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params + tracepoint_params).join(', ')});"
  puts '  return _retval;' if c.has_return_type?
end

def normal_wrapper(c, provider)
  puts "#{c.decl} {"
  puts '  _init_tracer();' if c.init?
  common_block(c, provider)
  puts '}'
  puts ''
end

def define_and_find_mpi_symbols
  $mpi_commands.each do |c|
    puts <<~EOF
      #define #{MPI_POINTER_NAMES[c]} #{c.pointer_name}
      #{c.decl_pointer(c.pointer_type_name)};
      static #{c.pointer_type_name} #{MPI_POINTER_NAMES[c]} = (void *) 0x0;

    EOF
  end

  puts 'static void find_mpi_symbols(void * handle, int verbose) {'
  $mpi_commands.each do |c|
    puts <<EOF

  #{MPI_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{MPI_POINTER_NAMES[c]} && verbose)
    fprintf(stderr, "THAPI: Missing symbol #{c.name}!\\n");
EOF
  end

  puts '}'
  puts ''
end

puts <<~EOF
  #include <stdint.h>
  #include <mpi.h>
  #include "mpi_tracepoints.h"
  #include <dlfcn.h>
  #include <pthread.h>
EOF

define_and_find_mpi_symbols

puts File.read(File.join(SRC_DIR, 'tracer_mpi_helpers.include.c'))

$mpi_commands.each do |c|
  normal_wrapper(c, :lttng_ust_mpi)
end
