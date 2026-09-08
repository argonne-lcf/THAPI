require_relative 'mpi_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_mpi, COMMANDS,
                          include: '#include <mpi.h.include>')
