require_relative 'opencl_model'
require_relative 'opencl_tracepoints'

puts <<~EOF
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
  puts <<~EOF
    TRACEPOINT_ENUM(
      lttng_ust_opencl,
      cl_bool,
      TP_ENUM_VALUES(
        ctf_enum_value("CL_FALSE", 0)
        ctf_enum_value("CL_TRUE", 1)
      )
    )

  EOF

  ENUMS.each do |name, e|
    name = e['type_name'] if e['type_name']
    puts <<~EOF
      TRACEPOINT_ENUM(
        lttng_ust_opencl,
        #{name},
        TP_ENUM_VALUES(
          #{e['values'].collect do |k, v| # {' '}
            "ctf_enum_value(\"#{k}\", #{"(#{name})" unless e['type_name']}#{v})"
          end.join("\n    ")}
        )
      )

    EOF
  end
end

tracepoint_lambda = lambda { |c, dir|
  event = {}
  event['name'] = c.prototype.name
  args = []
  unless c.parameters.size == 1 && c.parameters.first.decl.strip == 'void' && !((HOST_PROFILE || c.prototype.has_return_type?) && dir == 'stop')
    unless c.parameters.size == 1 && c.parameters.first.decl.strip == 'void'
      args = c.parameters.collect do |p|
        [p.callback? ? 'void *' : p.decl_pointer.gsub('[]', '*'),
         p.name]
      end
    end
    if dir == 'stop'
      args.push [c.prototype.return_type, '_retval'] if c.prototype.has_return_type?
      args.push %w[uint64_t _duration] if HOST_PROFILE
    end
    args += c.tracepoint_parameters.collect do |p|
      [p.type, p.name]
    end
  end
  event['args'] = args

  fields = []
  if dir == 'start'
    c.parameters.collect(&:lttng_in_type).compact.each do |arr|
      fields.push arr
    end
    c.meta_parameters.collect(&:lttng_in_type).compact.each do |arr|
      fields.push arr
    end
  elsif dir == 'stop'
    r = c.prototype.lttng_return_type
    fields.push r if r
    c.meta_parameters.collect(&:lttng_out_type).compact.each do |arr|
      fields.push arr
    end
    fields.push %w[ctf_integer uint64_t _duration _duration] if HOST_PROFILE
  end
  event[dir] = fields

  print_tracepoint('lttng_ust_opencl', event, dir)
}

$opencl_commands.each do |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS

  tracepoint_lambda.call(c, 'start')
  tracepoint_lambda.call(c, 'stop')
end

$opencl_extension_commands.each do |c|
  next if c.parameters.length > LTTNG_USABLE_PARAMS

  tracepoint_lambda.call(c, 'start')
  tracepoint_lambda.call(c, 'stop')
end

puts ''

namespace = 'lttng_ust_opencl'
callbacks = YAML.load_file(File.join(SRC_DIR, 'opencl_wrapper_events.yaml'))[namespace]
callbacks['events'].each do |e|
  %w[start stop].each do |dir|
    print_tracepoint(namespace, e, dir)
  end
end
