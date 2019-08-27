require_relative 'opencl_model'

provider = :lttng_ust_opencl


puts <<EOF
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
EOF

$opencl_commands.each { |c|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{c.prototype.name}_start,
  TP_ARGS(
EOF
  print "    "
  if c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
    print "void"
  else
    puts c.parameters.collect { |p|
      "#{p.callback? ? "void *" : p.decl_pointer.gsub("const","").gsub("[]", "*")}, #{p.name}"
    }.join(",\n    ")
  end
  puts <<EOF
  ),
  TP_FIELDS(
  )
)

EOF
}

puts <<EOF
EOF
