RESULT_NAME = "zeResult"

$tracepoint_lambda = lambda { |provider, c, dir|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{c.name}_#{SUFFIXES[dir]},
  TP_ARGS(
EOF
  print "    "
  if (c.parameters.nil? || c.parameters.empty?) && !(c.has_return_type? && dir == :stop)
    print "void"
  else
    params = []
    params.concat c.parameters.collect { |p|
      "#{p.type}, #{p.name}"
    } unless c.parameters.nil? || c.parameters.empty?
    if c.has_return_type? && dir == :stop
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
  if dir == :start
    if c.parameters
      c.parameters.collect(&:lttng_type).compact.each { |r|
        fields.push(r.call_string)
      }
    end
    c.meta_parameters.collect(&:lttng_in_type).flatten.compact.each { |r|
      fields.push(r.call_string)
    }
  elsif dir == :stop
    r = c.type.lttng_type
    if r
      r.name = RESULT_NAME
      r.expression = RESULT_NAME
      fields.push(r.call_string)
    end
    c.meta_parameters.collect(&:lttng_out_type).flatten.compact.each { |r|
      fields.push(r.call_string)
    }
  end
  puts "    " << fields.join("\n    ")
  puts <<EOF
  )
)

EOF
}

$struct_tracepoint_lambda = lambda { |provider, t|
  puts <<EOF
TRACEPOINT_EVENT(
  #{provider},
  #{t},
  TP_ARGS(
    #{t} *, p
  ),
  TP_FIELDS(
    ctf_integer_hex(uintptr_t, p, (uintptr_t)(p))
    ctf_sequence_text(uint8_t, p_val, p, size_t, (p ? sizeof(#{t}) : 0))
  )
)

EOF
}
