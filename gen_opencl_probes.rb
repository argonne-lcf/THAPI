require_relative 'opencl_model'

provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>

TRACEPOINT_EVENT(
  #{provider},
  event_profiling,
  TP_ARGS(
    int, status,
    cl_event, event
  ),
  TP_FIELDS(
    ctf_integer(int, status, status)
    ctf_integer_hex(intptr_t, event, event)
  )
)

TRACEPOINT_EVENT(
  #{provider},
  event_profiling_results,
  TP_ARGS(
    cl_event, event,
    cl_int, event_command_exec_status,
    cl_int, queued_status,
    cl_ulong, queued,
    cl_int, submit_status,
    cl_ulong, submit,
    cl_int, start_status,
    cl_ulong, start,
    cl_int, end_status,
    cl_ulong, end
  ),
  TP_FIELDS(
    ctf_integer_hex(intptr_t, event, event)
    ctf_integer(int, event_command_exec_status, event_command_exec_status)
    ctf_integer(cl_int, queued_status, queued_status)
    ctf_integer(cl_ulong, queued, queued)
    ctf_integer(cl_int, submit_status, submit_status)
    ctf_integer(cl_ulong, submit, submit)
    ctf_integer(cl_int, start_status, start_status)
    ctf_integer(cl_ulong, start, start)
    ctf_integer(cl_int, end_status, end_status)
    ctf_integer(cl_ulong, end, end)
  )
)
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

puts <<EOF
EOF
