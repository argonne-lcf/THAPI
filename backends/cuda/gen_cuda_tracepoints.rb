require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_cuda, COMMANDS,
                          include: '#include <cuda.h.include>')
