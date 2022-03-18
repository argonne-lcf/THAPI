RESULT_NAME = "ompResult"

$tracepoint_lambda = lambda { |provider, c|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{c.name.gsub(/_func\z/,"")},
  TP_ARGS(
EOF
  print "    "
  if (c.parameters.nil? || c.parameters.empty?)
    print "void"
  else
    params = []
    params.concat c.parameters.collect { |p|
      "#{p.type}, #{p.name}"
    } unless c.parameters.nil? || c.parameters.empty?
    if c.has_return_type?
      params.push("#{c.type}, #{RESULT_NAME}")
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
  c.parameters.collect(&:lttng_type).compact.each { |r|
    fields.push(r.call_string)
  }
  c.meta_parameters.collect(&:lttng_in_type).flatten.compact.each { |r|
    fields.push(r.call_string)
  }
  puts "    " << fields.join("\n    ")
  puts <<EOF
  )
)

EOF
}
