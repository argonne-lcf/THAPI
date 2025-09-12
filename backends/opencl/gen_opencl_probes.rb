require_relative 'opencl_model'
require_relative 'opencl_tracepoints'

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 300
#include "lttng/tracepoint_gen.h"
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include "./tracer_opencl.h"

EOF

if GENERATE_ENUMS_TRACEPOINTS
  puts <<EOF
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
end

tracepoint_lambda = lambda { |c, dir|
  event = {}
  event["name"] = c.prototype.name
  args = []
  unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void" && !((HOST_PROFILE || c.prototype.has_return_type?) && dir == "stop")
    args = c.parameters.collect { |p|
      [p.callback? ? "void *" : p.decl_pointer.gsub("[]", "*"),
       p.name]
    } unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
    if dir == "stop"
      args.push [c.prototype.return_type, "_retval"] if c.prototype.has_return_type?
      args.push ["uint64_t", "_duration"] if HOST_PROFILE
    end
    args += c.tracepoint_parameters.collect { |p|
      [p.type, p.name]
    }
  end
  event["args"] = args

  fields = []
  if dir == "start"
    c.parameters.collect(&:lttng_in_type).compact.each { |arr|
      fields.push arr
    }
    c.meta_parameters.collect(&:lttng_in_type).compact.each { |arr|
      fields.push arr
    }
  elsif dir == "stop"
    r = c.prototype.lttng_return_type
    fields.push r if r
    c.meta_parameters.collect(&:lttng_out_type).compact.each { |arr|
      fields.push arr
    }
    if HOST_PROFILE
      fields.push ["ctf_integer", "uint64_t", "_duration", "_duration"]
    end
  end
  event[dir] = fields

  print_tracepoint("lttng_ust_opencl", event, dir)
}

$opencl_commands.each { |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, "start")
  tracepoint_lambda.call(c, "stop")
}

$opencl_extension_commands.each { |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, "start")
  tracepoint_lambda.call(c, "stop")
}

puts <<EOF
EOF

namespace = "lttng_ust_opencl"
callbacks = YAML::load_file(File.join(SRC_DIR,"opencl_wrapper_events.yaml"))[namespace]
callbacks["events"].each { |e|
  ["start", "stop"].each { |dir|
    print_tracepoint(namespace, e, dir)
  }
}
