require_relative 'mpi_model'
require_relative '../../utils/gen_tracer_base'

def normal_wrapper(c, provider)
  print_wrapper(c, init: ('_init_tracer();' if c.init?)) do
    print_traced_body(c, provider, MPI_POINTER_NAMES, epilogues: :after_exit)
  end
end

def define_and_find_mpi_symbols
  print_pointer_table(COMMANDS, MPI_POINTER_NAMES, blank: :after,
                                                   before: ->(c) { pointer_define(c, MPI_POINTER_NAMES) })

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
