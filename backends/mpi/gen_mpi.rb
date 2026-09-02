require_relative 'mpi_model'
require_relative '../../utils/gen_tracer_base'

def common_block(c, provider)
  call_args = tracepoint_call_args(c)
  print_tracepoint_locals(c)
  print_tracepoint_call(provider, c, :start, call_args)

  print_traced_call(c, MPI_POINTER_NAMES[c])
  c.tracepoint_parameters.each { |p| puts p.init if p.after? }

  call_args.push '_retval' if c.has_return_type?
  print_tracepoint_call(provider, c, :stop, call_args)

  c.epilogues.each do |p|
    puts p
  end
end

def normal_wrapper(c, provider)
  print_wrapper(c, init: ('_init_tracer();' if c.init?)) { common_block(c, provider) }
end

def define_and_find_mpi_symbols
  COMMANDS.each do |c|
    puts <<~EOF
      #define #{MPI_POINTER_NAMES[c]} #{c.pointer_name}
      #{c.decl_pointer(c.pointer_type_name)};
      static #{c.pointer_type_name} #{MPI_POINTER_NAMES[c]} = (void *) 0x0;

    EOF
  end

  puts 'static void find_mpi_symbols(void * handle, int verbose) {'
  print_dlsym_lookups(COMMANDS, MPI_POINTER_NAMES, prefix: 'THAPI: ')
  puts '}'
  puts ''
end

puts <<~EOF
  #include <stdint.h>
  #define MPICH_FORTRAN_SYMBOLS_NONABI
  #include <mpi.h>
  #include "mpi_tracepoints.h"
  #include "mpi_type.h"
  #include <dlfcn.h>
  #include <pthread.h>
EOF

define_and_find_mpi_symbols

puts File.read(File.join(SRC_DIR, 'tracer_mpi_helpers.include.c'))

COMMANDS.each do |c|
  normal_wrapper(c, :lttng_ust_mpi)
end
