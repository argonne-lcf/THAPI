require_relative 'ze_model'

namespace = ARGV[0]

raise "No namespace provided!" unless namespace

h = YAML::load_file(File.join(SRC_DIR,"ze_events.yaml"))[namespace]

raise "Invalid namespace!" unless h

puts <<EOF
#include "ze.h.include"

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
