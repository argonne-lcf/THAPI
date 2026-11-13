require_relative 'libfabric_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_libfabric, COMMANDS,
                          include: '#include <rdma/fi_endpoint.h>')
