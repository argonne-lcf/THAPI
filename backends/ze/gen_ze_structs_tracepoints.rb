require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

print_struct_tracepoint_provider(:lttng_ust_ze_structs, tagged_structs(APIS[:ze]),
                                 include: '#include "ze.h.include"')
