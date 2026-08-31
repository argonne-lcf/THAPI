# The whole of a tracepoint-provider generator: an include line, then one pair
# of entry/exit tracepoints per traced function.
#
# `directions` is what a backend wires an event to. Five APIs trace a call's
# entry and its return separately, so a function becomes two tracepoints.
# itt and ompt are callback APIs -- the runtime calls in once, there is no
# return to observe -- so they emit a single undirected tracepoint.
#
# A function with more parameters than LTTng can carry is skipped: the macro
# would not compile, and dropping the event is better than failing the build.
def print_tracepoint_provider(provider, commands, include:, directions: %i[start stop])
  puts <<~EOF
    #include "lttng/tracepoint_gen.h"
    #{include}
  EOF

  # A provider that also declares events of its own, not derived from a traced
  # function, emits them here -- before the generated ones, so its enums are in
  # scope for them.
  yield if block_given?

  commands.groups[provider].each do |c|
    next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

    directions.each { |dir| print_tracepoint(provider, c, dir) }
  end
end

# The event a tracepoint declares and the event the tracer fires have to be
# spelled the same way, so both sides read the name from here.
#
# `dir` is :start or :stop for an API whose calls have an entry and a return to
# observe, and nil for a callback API: the runtime calls in once, so there is
# one event, and its name drops the _func suffix the callback typedef carries.
def tracepoint_event_name(c, dir)
  dir ? "#{c.name}_#{SUFFIXES[dir]}" : c.name.gsub(/_func\z/, '')
end

def print_tracepoint(provider, c, dir = nil)
  name = tracepoint_event_name(c, dir)

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
    params.push("#{c.type}, #{c.result_name}") if c.has_return_type? && dir != :start
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

  # Add Result
  r = c.type.lttng_type(c.type_classes)
  if dir != :start && r
    r.name = c.result_name
    r.expression = if c.type.is_a?(YAMLCAst::Struct) || c.type.is_a?(YAMLCAst::Union)
                     "&#{c.result_name}"
                   else
                     c.result_name
                   end
    fields.push(r)
  end

  # Add parameters
  fields += c.parameters.collect { |p| p.lttng_type(c.type_classes) } if dir != :stop && c.parameters

  # Add meta parameteter
  name = if dir == :start
           :lttng_in_type
         elsif dir == :stop
           :lttng_out_type
         else
           :lttng_type
         end

  fields += c.meta_parameters.collect(&name).flatten

  puts '    ' << fields.compact.map(&:call_string).join("\n    ")
  puts <<~EOF
      )
    )

  EOF
end

# The struct-payload counterpart of print_tracepoint_provider: ze traces the
# bytes of the extension structs a call was handed, one tracepoint per struct.
def print_struct_tracepoint_provider(provider, structs, include:)
  puts <<~EOF
    #include "lttng/tracepoint_gen.h"
    #{include}
  EOF

  structs.each { |t| print_struct_tracepoint(provider, t) }
end

def print_struct_tracepoint(provider, t)
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
end
