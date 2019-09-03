require_relative 'opencl_model'

provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
EOF


tracepoint_lambda = lambda { |c, dir|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{c.prototype.name}_#{dir},
  TP_ARGS(
EOF
  print "    "
  if c.parameters.size == 1 && c.parameters.first.decl.strip == "void" && !(c.prototype.has_return_type? && dir == :stop)
    print "void"
  else
    params = []
    params << c.parameters.collect { |p|
      "#{p.callback? ? "void *" : p.decl_pointer.gsub("[]", "*")}, #{p.name}"
    } unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
    if c.prototype.has_return_type? && dir == :stop
      params.push("#{c.prototype.return_type}, _retval");
    end
    puts params.join(",\n    ")
  end
  puts <<EOF
  ),
  TP_FIELDS(
  )
)

EOF
}

$opencl_commands.each { |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, :start)
  tracepoint_lambda.call(c, :stop)
}

puts <<EOF
EOF
