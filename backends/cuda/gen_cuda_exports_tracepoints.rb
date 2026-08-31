require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'

PROVIDER = :lttng_ust_cuda_exports

# cuda_exports traces the driver's own export table, whose events are not
# derived from a header prototype: they are written out in cuda_events.yaml.
print_tracepoint_provider(PROVIDER, COMMANDS, include: '#include <cuda.h.include>') do
  declared = YAML.load_file(File.join(SRC_DIR, 'cuda_events.yaml'))[PROVIDER.to_s]
  declared['enums']&.each { |e| LTTng.print_enum(PROVIDER, e) }
  declared['events'].each { |e| LTTng.print_tracepoint(PROVIDER, e) }
end
