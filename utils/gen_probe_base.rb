$tracepoint_lambda = lambda { |provider, c, dir = nil|
  name = if dir
           "#{c.name}_#{SUFFIXES[dir]}"
         # OMP backend
         else
           c.name.gsub(/_func\z/, '')
         end

  puts <<~EOF
    TRACEPOINT_EVENT(
      #{provider},
      #{name},
      TP_ARGS(
  EOF
  print '    '
  if (c.parameters.nil? || c.parameters.empty?) && !(c.has_return_type? && dir != :start)
    print 'void'
  else
    params = []
    unless c.parameters.nil? || c.parameters.empty?
      params.concat(c.parameters.collect do |p|
        "#{p.type.to_s.gsub(/\[.*\]/, '*')}, #{p.name}"
      end)
    end
    params.push("#{c.type}, #{RESULT_NAME}") if c.has_return_type? && dir != :start
    params += c.tracepoint_parameters.reject { |p| p.after? && dir == :start }.collect do |p|
      "#{p.type.to_s.gsub(/\[.*\]/, '*')}, #{p.name}"
    end
    puts params.join(",\n    ")
  end
  puts <<EOF
  ),
  TP_FIELDS(
EOF
  fields = []
  if dir == :start
    if c.parameters
      c.parameters.collect(&:lttng_type).compact.each do |r|
        fields.push(r.call_string)
      end
    end
    c.meta_parameters.collect(&:lttng_in_type).flatten.compact.each do |r|
      fields.push(r.call_string)
    end
  elsif dir == :stop
    r = c.type.lttng_type
    if r
      r.name = RESULT_NAME
      r.expression = if c.type.is_a?(YAMLCAst::Struct) || c.type.is_a?(YAMLCAst::Union)
                       "&#{RESULT_NAME}"
                     else
                       RESULT_NAME
                     end
      fields.push(r.call_string)
    end
    c.meta_parameters.collect(&:lttng_out_type).flatten.compact.each do |r|
      fields.push(r.call_string)
    end
  else
    fields = []
    if r
      r.name = RESULT_NAME
      r.expression = if c.type.is_a?(YAMLCAst::Struct) || c.type.is_a?(YAMLCAst::Union)
                       "&#{RESULT_NAME}"
                     else
                       RESULT_NAME
                     end
      fields.push(r.call_string)
    end
    c.parameters.collect(&:lttng_type).compact.each do |r|
      fields.push(r.call_string)
    end
    c.meta_parameters.collect(&:lttng_type).flatten.compact.each do |r|
      fields.push(r.call_string)
    end
  end
  puts '    ' << fields.join("\n    ")
  puts <<~EOF
      )
    )

  EOF
}

$struct_tracepoint_lambda = lambda { |provider, t|
  puts <<~EOF
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
