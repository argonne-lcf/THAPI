require 'yaml'
cuda_babeltrace_model = YAML::load_file("cuda_babeltrace_model.yaml")

puts <<EOF
#ifndef _BABELTRACE_CUDA_CALLBACKS_H
#define _BABELTRACE_CUDA_CALLBACKS_H
#define __CUDA_API_VERSION_INTERNAL 1
#include <cuda.h>
#include <babeltrace2/babeltrace.h>

EOF


cuda_babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  fields = klass[:payload]
  decls = []
  fields.each { |f|
    decls.push ['size_t', "_#{f[:name]}_length"] if f[:class] == 'array_static'
    decls.push [f[:cast_type], f[:name]]
  }
  puts <<EOF
typedef void (#{name.gsub(":","_")}_cb)(
  #{(["const bt_event *bt_evt", "const bt_clock_snapshot *bt_clock"]+
    decls.collect { |t, n| "#{t} #{n}" }).join(",\n  ")}
);

EOF
}

puts <<EOF
#endif
EOF
