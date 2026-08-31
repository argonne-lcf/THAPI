require_relative 'itt_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_itt, COMMANDS,
                          include: '#include "ittnotify.h"',
                          directions: [nil])
