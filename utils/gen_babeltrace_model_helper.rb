require_relative 'yaml_ast'
require_relative 'type_registry'

# Build the babeltrace TypeRegistry from the backend's API model. Whether an
# API has bitfield types is a fact about its headers, so each backend states
# what it expects: a backend that silently stopped classifying them would
# otherwise emit plain integers where enum metadata belongs.
def build_ast_registry(naming, backend, expect_bitfields:)
  api = naming.api
  registry = TypeRegistry.new(
    all_types: api.types, all_enums: api.enums,
    enum_names: api.enum_names, bitfield_names: api.bitfield_names, struct_names: api.struct_names,
    class_namer: ->(name) { naming.scoped_class_name(name) }
  )
  if expect_bitfields
    raise "#{backend}: expected bitfield types" if registry.bitfield_names.empty?
  else
    raise "#{backend}: expected no bitfield types" unless registry.bitfield_names.empty?
  end
  registry
end

def meta_parameter_types_name(m, dir = nil)
  lttng = if dir == :start
            m.lttng_in_type
          elsif dir == :stop
            m.lttng_out_type
          else
            m.lttng_type
          end
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    else
      [[lttng.macro.to_s, "#{t}", "#{name}", lttng]]
    end
  when ArrayMetaParameter, InString, OutString, OutLTTng, InLTTng, ReturnString
    if lttng.macro.to_s == 'ctf_string'
      [['ctf_string', "#{t} *", "#{name}", lttng]]
    else
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  when ArrayByRefMetaParameter
    [['ctf_integer', 'size_t', "_#{name}_length", nil],
     [lttng.macro.to_s, "#{t.type} *", "#{name}", lttng]]
  when FixedArrayMetaParameter
    [[lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
  when OutPtrString
    [['ctf_string', "#{t}", "#{name}", lttng]]
  else
    raise "unsupported meta parameter class #{m.class} #{lttng.call_string} #{t}"
  end
end

def get_extra_fields_types_name(event)
  event['fields'].collect do |field|
    lttng = LTTng::TracepointField.new(*field)
    name = lttng.name.to_s
    type = event['args'].find { |_t, n| n == name || n == name.gsub(/_vals?\z/, '') }[0]
    case lttng.macro.to_s
    when /ctf_sequence/
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, type, name, lttng]]
    else
      [[lttng.macro.to_s, type, name, lttng]]
    end
  end.flatten(1)
end

def gen_bt_field_model(registry, lttng_name, type, name, lttng)
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
    if registry.enum_names.include?(type) || registry.bitfield_names.include?(type)
      member[:metadata] = { be_class: registry.class_namer.call(type) }
    end
  when 'ctf_sequence', 'ctf_sequence_hex'
    array_type = lttng.type.to_s
    field[:type] = 'array_dynamic'
    field[:element_field_class] =
      { type: registry.integer_signed?(array_type) ? 'integer_signed' : 'integer_unsigned',
        field_value_range: registry.integer_size(array_type) }

    field[:element_field_class][:preferred_display_base] = 16 if lttng_name.end_with?('_hex')

    match = type.match(/(.*) \*/)

    field[:element_field_class][:cast_type] = match[1]
    field[:length_field_path] = "EVENT_PAYLOAD[\"_#{name}_length\"]"
  when 'ctf_array', 'ctf_array_hex'
    array_type = lttng.type.to_s
    field[:type] = 'array_static'
    field[:element_field_class] =
      { type: registry.integer_signed?(array_type) ? 'integer_signed' : 'integer_unsigned',
        field_value_range: registry.integer_size(array_type) }
    field[:element_field_class][:preferred_display_base] = 16 if lttng_name.end_with?('_hex')
    field[:length] = lttng.length
  when 'ctf_string'
    field[:type] = 'string'
  when 'ctf_sequence_text', 'ctf_array_text'
    field[:type] = 'string'
    t = type.sub(' *', '')
    t = types_by_name[t].type.name while types_by_name.include?(t) && types_by_name[t].type.is_a?(YAMLCAst::CustomType)
    member[:metadata] = { be_class: registry.class_namer.call(t) } if registry.struct_names.include?(t)

    # Too complicated, not sure why `all_struct_names` is not enough
    if !field[:cast_type].end_with?('*') && (registry.struct_names.include?(t) || types_by_name[t]&.type.is_a?(YAMLCAst::Union) || type.start_with?('struct'))
      field[:cast_type_is_struct] = true
    end
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
      [lttng.macro.to_s, p.type.to_s, p.name.to_s, lttng]
    end
  end

  name = case dir
         when :start then In
         when :stop  then Out
         end

  fields + c.meta_parameters.select { |m| name.nil? || m.is_a?(name) }.collect do |m|
    meta_parameter_types_name(m, dir)
  end.flatten(1)
end

def gen_event_fields_bt_model(registry, c, dir)
  types_name = get_fields_types_name(c, dir)
  types_name.collect do |lttng_name, type, name, lttng|
    gen_bt_field_model(registry, lttng_name, type.sub(/\Aconst /, ''), name, lttng)
  end
end

def gen_extra_event_fields_bt_model(registry, event)
  types_name = get_extra_fields_types_name(event)
  types_name.collect do |lttng_name, type, name, lttng|
    gen_bt_field_model(registry, lttng_name, type.sub(/\Aconst /, ''), name, lttng)
  end
end

def gen_event_bt_model(registry, provider, c, dir = nil)
  d = if dir
        { name: "#{provider}:#{c.name}_#{SUFFIXES[dir]}" }
      # OMP backend
      else
        { name: "#{provider}:#{c.name.gsub(/_func\z/, '')}" }
      end

  m = gen_event_fields_bt_model(registry, c, dir)

  unless m.empty?
    d[:payload_field_class] =
      {
        type: 'structure',
        members: m,
      }
  end
  d
end

def gen_extra_event_bt_model(registry, provider, event)
  d = { name: "#{provider}:#{event['name']}" }
  m = gen_extra_event_fields_bt_model(registry, event)

  unless m.empty?
    d[:payload_field_class] =
      {
        type: 'structure',
        members: m,
      }
  end
  d
end

# itt and omp trace a single event per command; everyone else a start/stop pair.
def gen_command_events_bt_model(registry, provider_commands, phased: true)
  provider_commands.collect do |provider, commands|
    commands.collect do |c|
      if phased
        [gen_event_bt_model(registry, provider, c, :start),
         gen_event_bt_model(registry, provider, c, :stop)]
      else
        [gen_event_bt_model(registry, provider, c)]
      end
    end
  end.flatten(2)
end

def gen_extra_events_bt_model(registry, filename)
  YAML.load_file(File.join(SRC_DIR, filename)).collect do |provider, es|
    es['events'].collect do |event|
      gen_extra_event_bt_model(registry, provider, event)
    end
  end.flatten
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
