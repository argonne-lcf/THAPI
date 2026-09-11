require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

print_struct_tracepoint_provider(:lttng_ust_zes_structs, traced_structs(APIS[:zes]),
                                 include: '#include "ze.h.include"')
