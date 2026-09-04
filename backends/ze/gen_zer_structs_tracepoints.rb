require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

print_struct_tracepoint_provider(:lttng_ust_zer_structs, tagged_structs(APIS[:zer]),
                                 include: '#include "ze.h.include"')
