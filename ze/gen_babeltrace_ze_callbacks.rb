require 'yaml'
ze_babeltrace_model = YAML::load_file("ze_babeltrace_model.yaml")

puts <<EOF
#ifndef _BABELTRACE_ZE_CALLBACKS_H
#define _BABELTRACE_ZE_CALLBACKS_H
#include <ze_api.h>
#include <ze_ddi.h>
#include <zet_api.h>
#include <zet_ddi.h>
#include <zes_api.h>
#include <zes_ddi.h>
#include <layers/zel_tracing_api.h>
#include <layers/zel_tracing_ddi.h>
#include <babeltrace2/babeltrace.h>

EOF


ze_babeltrace_model[:event_classes].each { |klass|
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
