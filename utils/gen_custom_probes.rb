require_relative 'gen_probe_base'

events_path, namespace, header = ARGV

raise 'No events to load!' unless events_path
raise 'No namespace provided!' unless namespace

declared = declared_events(events_path, namespace)

['lttng/tracepoint_gen.h', header].compact.each do |h|
  puts h.start_with?('<') ? %(#include #{h}) : %(#include "#{h}")
end

puts

print_declared_events(namespace, declared)
