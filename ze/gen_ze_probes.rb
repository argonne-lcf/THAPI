require_relative 'ze_model'

provider = :lttng_ust_ze

puts <<EOF
#include <core/ze_api.h>
EOF

tracepoint_lambda = lambda { |c, dir|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{c.name}_#{dir},
  TP_ARGS(
EOF
  print "    "
  if (c.parameters.nil? || c.parameters.empty?) && !(c.type.has_return_type? && dir == :stop)
    print "void"
  else
    params = []
    params << c.parameters.collect { |p|
      "#{p.type}, #{p.name}"
    } unless c.parameters.nil?
    if c.has_return_type? && dir == :stop
      params.push("#{c.type}, ze_result")
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
    c.parameters.collect(&:lttng_type).compact.each { |r|
      fields.push(r.call_string)
    }
  elsif dir == :stop
    r = c.type.lttng_type
    if r
      r.name = "ze_result"
      r.expression = "ze_result"
      fields.push(r.call_string)
    end
  end
  puts "    " << fields.join("\n    ")
  puts <<EOF
  )
)

EOF
}

$ze_commands.each { |c|
  next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS
  tracepoint_lambda.call(c, :start)
  tracepoint_lambda.call(c, :stop)
}


