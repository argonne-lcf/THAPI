require_relative 'opencl_model'
require_relative 'opencl_tracepoints'

namespace = ARGV[0]

raise "No namespace provided!" unless namespace

h = YAML::load_file("opencl_events.yaml")[namespace]

raise "Invalid namespace!" unless h

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>

EOF
enums = h["enums"]

if enums
  enums.each { |e|
    print_enum(namespace, e)
  }
end

h["events"].each { |e|
  print_tracepoint(namespace, e)
}
