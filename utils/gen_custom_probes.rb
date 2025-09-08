require 'yaml'
require_relative 'LTTng'

events_path = ARGV[0]
namespace = ARGV[1]
header = ARGV[2]

raise 'No events to load!' unless events_path
raise 'No namespace provided!' unless namespace

h = YAML.load_file(events_path)[namespace]

raise "Invalid namespace: #{namespace}!" unless h

['lttng/tracepoint_gen.h', header].filter.each do |h|
  puts %(#include "#{h}")
end

puts

h['enums'].to_a.each do |e|
  LTTng.print_enum(namespace, e)
end

h['events'].to_a.each do |e|
  LTTng.print_tracepoint(namespace, e)
end
