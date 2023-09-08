require 'yaml'
require_relative '../utils/LTTng'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

namespace = ARGV[0]

raise "No namespace provided!" unless namespace

h = YAML::load_file(File.join(SRC_DIR,"sampling_events.yaml"))[namespace]

raise "Invalid namespace!" unless h

puts <<EOF
#include "lttng/tracepoint_gen.h"

EOF
enums = h["enums"]

if enums
  enums.each { |e|
    LTTng.print_enum(namespace, e)
  }
end

h["events"].each { |e|
  LTTng.print_tracepoint(namespace, e)
}
