require 'yaml'

require_relative 'gen_probe_base'
require_relative 'yaml_ast'
require_relative 'type_registry'
require_relative 'meta_parameters'

# Build the babeltrace TypeRegistry from the backend's API model. Whether an
# API has bitfield types is a fact about its headers, so each backend states
# what it expects: a backend that silently stopped classifying them would
# otherwise emit plain integers where enum metadata belongs.
def build_ast_registry(naming, expect_bitfields:)
  api = naming.api
  registry = TypeRegistry.new(
    all_types: api.types, all_enums: api.enums,
    enum_names: api.enum_names, bitfield_names: api.bitfield_names, struct_names: api.struct_names,
    class_namer: ->(name) { naming.scoped_class_name(name) }
  )
  if expect_bitfields
    raise "#{naming.backend}: expected bitfield types" if registry.bitfield_names.empty?
  else
    raise "#{naming.backend}: expected no bitfield types" unless registry.bitfield_names.empty?
  end
  registry
end

# The rows describing one field: the companion length field, when the field has
# one, then the field itself. Variable-length sequences and blobs are the two
# shapes lttng gives an implicit `_<name>_length`.
def implicit_length_field?(macro)
  macro.match?(/ctf_sequence/) || macro == 'lttng_ust_field_variable_length_blob'
end

def field_types_name(macro, type, name, lttng)
  rows = []
  rows << ['ctf_integer', 'size_t', length_field_name(name), nil] if implicit_length_field?(macro)
  rows << [macro, type, name, lttng]
  rows
end

# OutPtrString's parameter is already a pointer to the string, and a
# length-less scalar is read by value; everything else is traced by address.
def meta_parameter_types_name(m, dir)
  lttng = m.lttng_type_for(dir)
  decl_type = m.command[m.name].type
  # A literal row on a by-value parameter (opencl's _param_name).
  return field_types_name(lttng.macro.to_s, decl_type.to_s, lttng.name.to_s, lttng) unless decl_type.respond_to?(:type)

  t = decl_type.type
  t = t.type if m.is_a?(ArrayByRefMetaParameter)
  by_value = m.is_a?(OutPtrString) || (m.is_a?(ScalarMetaParameter) && !lttng.length)

  field_types_name(lttng.macro.to_s, by_value ? t.to_s : "#{t} *", lttng.name.to_s, lttng)
end

def get_extra_fields_types_name(event, phase = 'fields')
  event[phase].to_a.collect do |field|
    lttng = LTTng::TracepointField.new(*field)
    name = lttng.name.to_s
    type = LTTng.argument_type(event['args'], name)
    lttng.blob_type = type
    field_types_name(lttng.macro.to_s, type, name, lttng)
  end.flatten(1)
end

# The element description an array field carries, shared by the static and
# dynamic cases. Keys are inserted in the order the model YAML prints them.
def element_field_class(registry, lttng, lttng_name)
  array_type = lttng.type.to_s
  fc = { type: registry.integer_signed?(array_type) ? 'integer_signed' : 'integer_unsigned',
         field_value_range: registry.integer_size(array_type) }
  fc[:preferred_display_base] = 16 if lttng_name.end_with?('_hex')
  fc
end

# Raw bytes -- whether recorded as text or as a blob -- may really be a struct.
# Resolve the typedef chain to its underlying name, and when that names a struct
# say which FFI class reads it back.
def name_packed_struct(registry, member, type)
  types_by_name = registry.types_by_name
  t = type.sub(' *', '')
  t = types_by_name[t].type.name while types_by_name.include?(t) && types_by_name[t].type.is_a?(YAMLCAst::CustomType)
  member[:metadata] = { be_class: registry.class_namer.call(t) } if registry.struct_names.include?(t)
end

# `lttng_enums` maps the name of each LTTng enum the provider declares to its
# [macro, label, value] rows, for a ctf_enum field.
# An integer whose type is an enum or a set of flags names the class that
# reads it back, whether the field holds one or an array of them.
def name_enum(registry, member, type)
  return unless registry.enum_names.include?(type) || registry.bitfield_names.include?(type)

  member[:metadata] = { be_class: registry.class_namer.call(type) }
end

def gen_bt_field_model(registry, lttng_name, type, name, lttng, lttng_enums: {})
  types_by_name = registry.types_by_name
  member = { name: name }

  field = { cast_type: type.gsub(/\[.*\]/, '*') }
  if types_by_name[type].is_a?(YAMLCAst::Declaration) && types_by_name[type].type.is_a?(YAMLCAst::Function)
    field[:cast_type] =
      "#{type} *"
  end

  case lttng_name
  when 'ctf_float'
    field[:type] = type == 'float' ? 'single' : type
  when 'ctf_integer', 'ctf_integer_hex'
    field[:type] = registry.integer_signed?(type) ? 'integer_signed' : 'integer_unsigned'
    field[:field_value_range] = registry.integer_size(type)
    field[:preferred_display_base] = 16 if lttng_name.end_with?('_hex')
    name_enum(registry, member, type)
  when 'ctf_enum'
    field[:type] = registry.integer_signed?(type) ? 'enumeration_signed' : 'enumeration_unsigned'
    field[:mappings] = lttng_enums.fetch(lttng.enum_name).map do |_macro, label, value|
      { label: label, integer_range_set: [[value, value]] }
    end
  when 'ctf_sequence', 'ctf_sequence_hex'
    # A sequence carries its length in a companion field, and its element type
    # is the pointed-to half of the member's own type.
    field[:type] = 'array_dynamic'
    field[:element_field_class] = element_field_class(registry, lttng, lttng_name)
    field[:element_field_class][:cast_type] = type.sub(/ ?\*\z/, '')
    name_enum(registry, member, field[:element_field_class][:cast_type])
    field[:length_field_location] = payload_length_field_location(name)
  when 'ctf_array', 'ctf_array_hex'
    field[:type] = 'array_static'
    field[:element_field_class] = element_field_class(registry, lttng, lttng_name)
    field[:length] = lttng.length
  when 'ctf_string', 'ctf_sequence_text', 'ctf_array_text'
    # Genuine text: char strings and char sequences/arrays.
    field[:type] = 'string'
    name_packed_struct(registry, member, type)
  when 'lttng_ust_field_fixed_length_blob', 'lttng_ust_field_variable_length_blob'
    # Raw bytes -- a struct, a union, or an opaque buffer. A fixed-length blob
    # knows its size; a variable-length one reads it from the companion length
    # field lttng emits alongside it.
    if lttng_name == 'lttng_ust_field_fixed_length_blob'
      field[:type] = 'blob_static'
      field[:length] = lttng.length
    else
      field[:type] = 'blob_dynamic'
      field[:length_field_location] = payload_length_field_location(name)
    end
    field[:media_type] = lttng.media_type
    name_packed_struct(registry, member, type)
  else
    raise "unsupported lttng type: #{lttng.inspect}"
  end
  member[:field_class] = field
  member
end

def get_fields_types_name(c, dir)
  fields = []

  r = c.type.lttng_type(c.type_classes)
  fields.push([r.macro.to_s, c.type.to_s, c.result_name, r]) if dir != :start && r

  if dir != :stop
    fields += c.parameters.to_a.collect do |p|
      lttng = p.lttng_type(c.type_classes)
      # A function pointer is traced as an address (function_pointer?).
      [lttng.macro.to_s, function_pointer?(p.type) ? 'void *' : p.type.to_s, p.name.to_s, lttng]
    end
  end

  # An entry event carries only the meta-parameters that describe an input, an
  # exit event only those that describe a result; an undirected event takes all.
  phase = case dir
          when :start then In
          when :stop  then Out
          end

  fields + c.meta_parameters.select { |m| phase.nil? || m.is_a?(phase) }.collect do |m|
    meta_parameter_types_name(m, dir)
  end.flatten(1)
end

# One event class: its name, qualified by the provider that declares it, and a
# field for each row describing its payload. `const` is a promise to the C
# compiler, not part of the traced type, so it is dropped here.
def gen_bt_event(registry, provider, name, types_name, lttng_enums: {})
  members = types_name.collect do |lttng_name, type, field_name, lttng|
    gen_bt_field_model(registry, lttng_name, type.sub(/\Aconst /, ''), field_name, lttng, lttng_enums: lttng_enums)
  end
  event = { name: "#{provider}:#{name}" }
  event[:payload_field_class] = { type: 'structure', members: members } unless members.empty?
  event
end

# The name has to match the one the tracepoint provider declares.
def gen_event_bt_model(registry, provider, c, dir = nil)
  gen_bt_event(registry, provider, tracepoint_event_name(c, dir), get_fields_types_name(c, dir))
end

def gen_extra_event_bt_model(registry, provider, event, lttng_enums: {})
  gen_bt_event(registry, provider, event['name'], get_extra_fields_types_name(event), lttng_enums: lttng_enums)
end

# One event class per event the command produces, which is the command's own
# answer -- the same one the tracepoint provider enumerates.
def gen_command_events_bt_model(registry, provider_commands)
  provider_commands.collect do |provider, commands|
    commands.collect do |c|
      c.directions.collect { |dir| gen_event_bt_model(registry, provider, c, dir) }
    end
  end.flatten(2)
end

def gen_extra_events_bt_model(registry, events_path)
  yaml_load_file_cached(events_path).collect do |provider, es|
    lttng_enums = es['enums'].to_a.to_h { |e| [e['name'], e['values']] }
    es['events'].collect do |event|
      if (SUFFIXES.keys.map(&:to_s) & event.keys).empty?
        gen_extra_event_bt_model(registry, provider, event, lttng_enums: lttng_enums)
      else
        # A wrapped call: one event class per phase, as print_declared_events.
        SUFFIXES.collect do |dir, suffix|
          gen_bt_event(registry, provider, "#{event['name']}_#{suffix}",
                       get_extra_fields_types_name(event, dir.to_s), lttng_enums: lttng_enums)
        end
      end
    end
  end.flatten
end

# The whole of an AST backend's babeltrace-model generator: classify the API's
# types, turn every traced command into event classes, add the events the
# backend declares by hand, and print the model.
#
# `extra_events_path` is the backend's declared-event file, absent for a
# backend that declares none. `expect_bitfields` says whether this API's headers
# have bitfield types at all.
#
# `extra_event_classes` is anything the backend derives itself: ze adds one
# event per self-describing struct, which no other backend has. It is handed
# the registry, so its fields go through the shared classifier.
def print_bt_model(naming, commands, expect_bitfields:, extra_events_path: nil,
                   extra_event_classes: ->(_registry) { [] })
  registry = build_ast_registry(naming, expect_bitfields: expect_bitfields)

  event_classes = gen_command_events_bt_model(registry, commands.groups)
  event_classes += gen_extra_events_bt_model(registry, extra_events_path) if extra_events_path
  event_classes += extra_event_classes.call(registry)

  puts YAML.dump(gen_yaml(event_classes, naming.backend))
end

def gen_yaml(event_classes, backend)
  {
    environment: { entries: [
      {
        name: 'hostname',
        type: 'string',
      },
    ] },
    stream_classes: [
      { name: "thapi_#{backend}",
        default_clock_class: {},
        packet_context_field_class: { type: 'structure', members: [
          {
            name: 'cpu_id',
            field_class: {
              type: 'integer_unsigned',
              cast_type: 'uint64_t',
              field_value_range: 32,
            },
          },
        ] },
        event_common_context_field_class: { type: 'structure', members: [
          {
            name: 'vpid',
            field_class: {
              type: 'integer_signed',
              cast_type: 'int64_t',
              field_value_range: 64,
            },
          },
          {
            name: 'vtid',
            field_class: {
              type: 'integer_unsigned',
              cast_type: 'uint64_t',
              field_value_range: 64,
            },
          },
        ] },
        event_classes: event_classes },
    ],
  }
end
