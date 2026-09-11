require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

print_struct_tracepoint_provider(:lttng_ust_zet_structs, traced_structs(APIS[:zet]),
                                 include: '#include "ze.h.include"')
