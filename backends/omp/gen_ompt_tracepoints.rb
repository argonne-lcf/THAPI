require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_ompt, COMMANDS,
                          include: '#include <ompt.h.include>')
