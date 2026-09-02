require 'yaml'
require_relative 'LTTng'

# The events a backend writes out by hand, rather than deriving them from a
# traced function's prototype. They live under the provider's name in a
# `<backend>_events.yaml`; a provider that has none is simply absent from it.
#
# Enums first: a declared event may name one.
def print_declared_events(provider, declared)
  return unless declared

  declared['enums'].to_a.each { |e| LTTng.print_enum(provider, e) }
  declared['events'].to_a.each { |e| LTTng.print_tracepoint(provider, e) }
end

# The whole of a tracepoint-provider generator. A provider's events come from
# two places and it may use either or both: the ones declared in `events_path`,
# and one pair of entry/exit tracepoints per traced function.
#
# `directions` is [nil] for a callback API, which has a single undirected
# event; see tracepoint_event_name.
#
# A function with more parameters than LTTng can carry is skipped: the macro
# would not compile, and dropping the event is better than failing the build.
def print_tracepoint_provider(provider, commands, include:, events_path: nil, directions: %i[start stop])
  puts <<~EOF
    #include "lttng/tracepoint_gen.h"
    #{include}
  EOF

  print_declared_events(provider, events_path && YAML.load_file(events_path)[provider.to_s])

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
