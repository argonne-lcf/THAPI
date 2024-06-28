require_relative 'mpi_model'

namespace = ARGV[0]

raise 'No namespace provided!' unless namespace

h = YAML.load_file(File.join(SRC_DIR, 'mpi_events.yaml'))[namespace]

raise "Invalid namespace: #{namespace}!" unless h

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "mpi.h.include"

EOF
enums = h['enums']

if enums
  enums.each do |e|
    LTTng.print_enum(namespace, e)
  end
end

h['events'].each do |e|
  LTTng.print_tracepoint(namespace, e)
end
