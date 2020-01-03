$tracepoint_lambda = lambda { |provider, c, dir|
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
