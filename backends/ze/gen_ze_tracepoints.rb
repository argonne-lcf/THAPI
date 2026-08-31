require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_ze, COMMANDS,
                          include: '#include "ze.h.include"')
