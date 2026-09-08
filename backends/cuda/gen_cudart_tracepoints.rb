require_relative 'cudart_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_cudart, COMMANDS,
                          include: '#include <cudart.h.include>')
