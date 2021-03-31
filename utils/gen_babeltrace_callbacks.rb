require 'yaml'
model = ARGV[0]
header = ARGV[1]
namespace = ARGV[2]

babeltrace_model = YAML::load_file(model)

puts <<EOF
#ifndef _BABELTRACE_#{namespace.upcase}_CALLBACKS_H
#define _BABELTRACE_#{namespace.upcase}_CALLBACKS_H
EOF

puts File.read(header) if File.exist?(header)

puts <<EOF
#include <babeltrace2/babeltrace.h>

EOF


babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  fields = klass[:payload]
  decls = []
  fields.each { |f|
    decls.push ['size_t', "_#{f[:name]}_length"] if f[:class] == 'array_static'
    decls.push [f[:cast_type], f[:name]]
  }
  puts <<EOF
typedef void (#{namespace}_#{name.gsub(":","_")}_cb)(
  #{(["const bt_event *bt_evt", "const bt_clock_snapshot *bt_clock"]+
    decls.collect { |t, n| "#{t} #{n}" }).join(",\n  ")}
);

EOF
}

puts <<EOF
#endif
EOF
