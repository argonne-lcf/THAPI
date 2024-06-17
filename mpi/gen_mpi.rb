require_relative 'mpi_model'

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
  _retval = #{MPI_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  else
    puts "  #{MPI_POINTER_NAMES[c]}(#{params.join(', ')});"
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
  puts "  _init_tracer();" if c.init?

  common_block(c, provider)
  puts <<~EOF
    }

  EOF
end

def define_and_find_mpi_symbols()

  $mpi_commands.each { |c|
    puts <<EOF
#define #{MPI_POINTER_NAMES[c]} #{c.pointer_name}
#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{MPI_POINTER_NAMES[c]} = (void *) 0x0;

EOF
}

  puts <<EOF

static void find_mpi_symbols(void * handle, int verbose) {
EOF
  $mpi_commands.each { |c|
     puts <<EOF

    #{MPI_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
    if (!#{MPI_POINTER_NAMES[c]} && verbose)
      fprintf(stderr, "THAPI: Missing symbol #{c.name}!\\n");
EOF
  }

puts <<EOF
}

EOF
end

puts <<~EOF
  #include <stdint.h>
  #include <mpi.h>
  #include "mpi_tracepoints.h"
  #include <dlfcn.h>
  #include <pthread.h>
EOF

define_and_find_mpi_symbols

puts File::read(File.join(SRC_DIR,"tracer_mpi_helpers.include.c"))

$mpi_commands.each do |c|
  normal_wrapper(c, :lttng_ust_mpi)
end
