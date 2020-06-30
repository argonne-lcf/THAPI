require 'yaml'
opencl_model = YAML::load_file("opencl_model.yaml")

puts <<EOF
#ifndef _HEADER_CALLBACKS_H
#define _HEADER_CALLBACKS_H
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

#include "babeltrace_cl.h"

EOF

opencl_model["events"].each { |name, fields|
  puts <<EOF
typedef void (#{name.gsub(":","_")}_cb)(
    #{(["const bt_event *bt_event", "const bt_clock_snapshot *bt_clock"]+fields.each.collect { |n, f|
  s =  "#{f["type"].gsub("cl_errcode", "cl_int")}"
  s << " *" if f["pointer"]
  s << " *" if f["array"]
  s << " *" if f["string"]
  s << " #{n}"
  if f["array"]
    ["size_t _#{n}_length", s]
  else
    s
  end
}).flatten.join(",\n    ")});

EOF
}

puts <<EOF
#endif
EOF
