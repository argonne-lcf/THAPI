require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_hip, COMMANDS,
                          include: '#include <hip.h.include>')
