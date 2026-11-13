require_relative 'yaml_ast'
require_relative 'LTTng'
require_relative 'meta_parameters'

# One provider's entry in a backend's `<backend>_events.yaml`: the enums and
# events it declares by hand.
def declared_events(path, provider)
  yaml_load_file_cached(path).fetch(provider.to_s) do
    raise "#{path} declares no events for #{provider}!"
  end
end

# The events a backend writes out by hand, rather than deriving them from a
# traced function's prototype. They live under the provider's name in a
# `<backend>_events.yaml`.
#
# Enums first: a declared event may name one.
#
# An event that wraps a call -- `start` and `stop` field lists instead of
# `fields` -- declares one tracepoint per phase, named like a traced call's.
def print_declared_events(provider, declared)
  declared['enums'].to_a.each { |e| LTTng.print_enum(provider, e) }
  declared['events'].to_a.each do |e|
    if SUFFIXES.keys.none? { |dir| e.key?(dir.to_s) }
      LTTng.print_tracepoint(provider, e)
    else
      SUFFIXES.each { |dir, suffix| LTTng.print_tracepoint(provider, e, dir.to_s, suffix: suffix) }
    end
  end
end

# The whole of a tracepoint-provider generator. A provider's events come from
# two places and it may use either or both: the ones declared in `events_path`,
# and one tracepoint per event a traced function produces.
#
# A function with more parameters than LTTng can carry is skipped: the macro
# would not compile, and dropping the event is better than failing the build.
#
# `preamble` is C that has to come before even lttng's header.
def print_tracepoint_provider(provider, commands, include:, events_path: nil, preamble: nil)
  puts preamble if preamble
  puts <<~EOF
    #include "lttng/tracepoint_gen.h"
    #{include}
  EOF

  print_declared_events(provider, declared_events(events_path, provider)) if events_path

  commands.groups[provider].each do |c|
    next if c.parameters && c.parameters.length > LTTNG_USABLE_PARAMS

    c.directions.each { |dir| print_tracepoint(provider, c, dir) }
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

# A sequence field ships its element count in a companion integer field, named
# here because the tracepoint, the model and the pretty-printer must agree.
def length_field_name(name)
  "_#{name}_length"
end

LENGTH_FIELD_PATTERN = /\A#{length_field_name('.*')}\z/

def length_field_name?(name)
  LENGTH_FIELD_PATTERN.match?(name.to_s)
end

# Where the model points a variable-length field at its companion length field.
# MIP 1 addresses it as a scope plus a path, replacing the MIP 0
# `EVENT_PAYLOAD["..."]` string.
def payload_length_field_location(name)
  { root_scope: 'BT_FIELD_LOCATION_SCOPE_EVENT_PAYLOAD', items: [length_field_name(name)] }
end

def print_tracepoint(provider, c, dir = nil)
  name = tracepoint_event_name(c, dir)

  puts <<~EOF
    TRACEPOINT_EVENT(
      #{provider},
      #{name},
      TP_ARGS(
  EOF
  decl = lambda do |p|
    t = p.type
    type = t.to_s.gsub(/\[.*\]/, '*')
    if c.xml_arg_spelling?
      type = 'void *' if t.is_a?(YAMLCAst::Pointer) && t.type.is_a?(YAMLCAst::Function)
      type = "#{t.type} *" if t.is_a?(YAMLCAst::Array) && !t.length
    end
    "#{type}, #{p.name}"
  end
  params = c.parameters.to_a.collect(&decl)
  params.push("#{c.type}, #{c.result_name}") if c.has_return_type? && dir != :start
  params.push('uint64_t, _duration') if c.duration? && dir == :stop
  params += c.tracepoint_parameters_for(dir).collect(&decl)

  # A tracepoint that carries nothing still has to say so: TP_ARGS(void).
  puts "    #{params.empty? ? 'void' : params.join(",\n    ")}"
  puts <<EOF
  ),
  TP_FIELDS(
EOF
  fields = []

  # Add Result
  r = c.type.lttng_type(c.type_classes)
  if dir != :start && r
    r.name = c.result_field
    r.expression = if c.type.is_a?(YAMLCAst::Struct) || c.type.is_a?(YAMLCAst::Union)
                     "&#{c.result_name}"
                   else
                     c.result_name
                   end
    fields.push(r)
  end

  # Add parameters
  fields += c.parameters.collect { |p| p.lttng_type(c.type_classes) } if dir != :stop && c.parameters

  fields += c.meta_parameters.collect { |m| m.lttng_type_for(dir) }.flatten
  if c.duration? && dir == :stop
    fields.push(LTTng::TracepointField.new('ctf_integer', 'uint64_t', '_duration',
                                           '_duration'))
  end

  rows = fields.compact.map(&:call_string)
  puts "    #{rows.join("\n    ")}" unless rows.empty?
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
  LTTng.print_tracepoint(provider, {
                           'name' => t,
                           'args' => [["#{t} *", 'p']],
                           'fields' => [
                             ['ctf_integer_hex', 'uintptr_t', 'p', '(uintptr_t)p'],
                             ['lttng_ust_field_variable_length_blob', 'p_val', 'p', 'size_t',
                              "(p ? sizeof(#{t}) : 0)"],
                           ],
                         })
end
