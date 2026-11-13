# One of ze's tracepoint providers, named as the build names its file:
# `<namespace>_tracepoints` for the functions of a namespace,
# `<namespace>_structs_tracepoints` for the extension structs it traces.
require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

namespace, structs = ARGV.fetch(0).match(/\A(\w+?)(_structs)?_tracepoints\z/).captures
include = %(#include "ze.h.include")
if structs
  print_struct_tracepoint_provider(:"lttng_ust_#{namespace}_structs", traced_structs(APIS.fetch(namespace.to_sym)),
                                   include: include)
else
  print_tracepoint_provider(:"lttng_ust_#{namespace}", COMMANDS, include: include)
end
