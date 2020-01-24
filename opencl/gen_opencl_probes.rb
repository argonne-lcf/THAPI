require_relative 'opencl_model'

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include "./tracer_opencl.h"

TRACEPOINT_ENUM(
  lttng_ust_opencl,
  cl_bool,
  TP_ENUM_VALUES(
    ctf_enum_value("CL_FALSE", 0)
    ctf_enum_value("CL_TRUE", 1)
  )
)

EOF

ENUMS.each { |name, e|
  name = e["type_name"] if e["type_name"]
  puts <<EOF
TRACEPOINT_ENUM(
  lttng_ust_opencl,
  #{name},
  TP_ENUM_VALUES(
    #{e["values"].collect { |k, v| "ctf_enum_value(\"#{k}\", #{e["type_name"]? "" : "(#{name})"}#{v})" }.join("\n    ")}
  )
)

EOF
}

tracepoint_lambda = lambda { |c, dir|
  puts <<EOF
TRACEPOINT_EVENT(
  lttng_ust_opencl,
  #{c.prototype.name}_#{dir},
  TP_ARGS(
EOF
  print "    "
  if c.parameters.size == 1 && c.parameters.first.decl.strip == "void" && !((HOST_PROFILE || c.prototype.has_return_type?) && dir == :stop)
    print "void"
  else
    params = []
    params << c.parameters.collect { |p|
      "#{p.callback? ? "void *" : p.decl_pointer.gsub("[]", "*")}, #{p.name}"
    } unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
    if dir == :stop
      params.push("#{c.prototype.return_type}, _retval") if c.prototype.has_return_type?
      params.push("uint64_t, _duration") if HOST_PROFILE
    end
    params += c.tracepoint_parameters.collect { |p|
      "#{p.type}, #{p.name}"
    }
    puts params.join(",\n    ")
  end
  puts <<EOF
  ),
  TP_FIELDS(
EOF
  fields = []
  if dir == :start
    c.parameters.collect(&:lttng_in_type).compact.each { |func, *args|
      fields.push("#{func}(#{args.join(", ")})")
    }
    c.meta_parameters.collect(&:lttng_in_type).compact.each { |func, *args|
      fields.push("#{func}(#{args.join(", ")})")
    }
  elsif dir == :stop
    r = c.prototype.lttng_return_type
    if r
      func, *args = r
      fields.push("#{func}(#{args.join(", ")})")
    end
    if HOST_PROFILE
      fields.push("ctf_integer(uint64_t, _duration, _duration)")
    end
    c.meta_parameters.collect(&:lttng_out_type).compact.each { |func, *args|
      fields.push("#{func}(#{args.join(", ")})")
    }
  end
  puts "    " << fields.join("\n    ")
  puts <<EOF
  )
)

EOF
}

$opencl_commands.each { |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, :start)
  tracepoint_lambda.call(c, :stop)
}

$opencl_extension_commands.each { |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, :start)
  tracepoint_lambda.call(c, :stop)
}

puts <<EOF
EOF

puts File::read("opencl_callbacks.tp.include")
