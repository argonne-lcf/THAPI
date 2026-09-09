require_relative 'mpi_model'
require_relative '../../utils/gen_tracer_base'

def define_and_find_mpi_symbols
  print_pointer_table(COMMANDS, MPI_POINTER_NAMES,
                      before: ->(c) { pointer_define(c, MPI_POINTER_NAMES) })

  print_find_symbols('mpi', COMMANDS, MPI_POINTER_NAMES, prefix: 'THAPI: ')
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

print_traced_wrappers(COMMANDS, :lttng_ust_mpi, MPI_POINTER_NAMES,
                      body_opts: ->(_c) { { epilogues: :after_exit } })
