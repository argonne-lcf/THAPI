require 'yaml'

if ARGV.empty?
  namespace = "babeltrace_cl"
else
  namespace = ARGV[0]
end

opencl_babeltrace_model = YAML::load_file("opencl_babeltrace_model.yaml")

puts <<EOF
#ifndef #{namespace.upcase}_HEADER_CALLBACKS_H
#define #{namespace.upcase}_HEADER_CALLBACKS_H
#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <CL/cl_ext_intel.h>
#include "tracer_opencl.h"

#include "#{namespace}.h"

EOF

opencl_babeltrace_model[:event_classes].each { |klass|
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
      decls.collect { |t, n| "#{t} #{n}" }).join(",\n    ")});

EOF
}

puts <<EOF
#endif
EOF
