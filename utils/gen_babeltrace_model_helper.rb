def get_extra_fields_types_name(event)
  event['fields'].collect do |field|
    lttng = LTTng::TracepointField.new(*field)
    name = lttng.name.to_s
    type = event['args'].find { |_t, n| n == name || n == name.gsub(/_val\z/, '') }[0]
    case lttng.macro.to_s
    when /ctf_sequence/
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, type, name, lttng]]
    else
      [[lttng.macro.to_s, type, name, lttng]]
    end
  end.flatten(1)
end

$types_by_name = $all_types.map { |ty| [ty.name, ty] }.to_h

def gen_bt_field_model(lttng_name, type, name, lttng)
  menber = { name: name }

  field = { cast_type: type.gsub(/\[.*\]/, '*') }
  if $types_by_name[type].is_a?(YAMLCAst::Declaration) && $types_by_name[type].type.is_a?(YAMLCAst::Function)
    field[:cast_type] =
      "#{type} *"
  end

  case lttng_name
  when 'ctf_float'
    field[:type] = type == 'float' ? 'single' : type
  when 'ctf_integer', 'ctf_integer_hex'
    field[:type] = integer_signed?(type) ? 'integer_signed' : 'integer_unsigned'
    field[:field_value_range] = integer_size(type)
    field[:preferred_display_base] = 16 if lttng_name.end_with?('_hex')
    if $all_enum_names.include?(type) || $all_bitfield_names.include?(type)
      menber[:metadata] = { be_class: to_scoped_class_name(type) }
    end
  when 'ctf_sequence', 'ctf_sequence_hex'
    array_type = lttng.type.to_s
    field[:type] = 'array_dynamic'
    field[:element_field_class] =
      { type: integer_signed?(array_type) ? 'integer_signed' : 'integer_unsigned',
        field_value_range: integer_size(array_type) }

    field[:element_field_class][:preferred_display_base] = 16 if lttng_name.end_with?('_hex')

    match = type.match(/(.*) \*/)

    field[:element_field_class][:cast_type] = match[1]
    field[:length_field_path] = "EVENT_PAYLOAD[\"_#{name}_length\"]"
  when 'ctf_array', 'ctf_array_hex'
    array_type = lttng.type.to_s
    field[:class] = 'array_static'
    field[:element_field_class] =
      { type: integer_signed?(array_type) ? 'integer_signed' : 'integer_unsigned',
        field_value_range: integer_size(array_type) }
    field[:element_field_class][:preferred_display_base] = 16 if lttng_name.end_with?('_hex')
    field[:length] = lttng.length
  when 'ctf_string'
    field[:type] = 'string'
  when 'ctf_sequence_text', 'ctf_array_text'
    field[:type] = 'string'
    t = type.sub(' *', '')
    while $types_by_name.include?(t) && $types_by_name[t].type.is_a?(YAMLCAst::CustomType)
      t = $types_by_name[t].type.name
    end
    if $all_struct_names.include?(t)
      menber[:metadata] = { be_class: to_scoped_class_name(t) }
      field[:cast_type_is_struct] = true unless type.include?('*')
    end
  else
    raise "unsupported lttng type: #{lttng.inspect}"
  end
  menber[:field_class] = field
  menber
end

def gen_event_fields_bt_model(c, dir)
  types_name = get_fields_types_name(c, dir)
  types_name.collect do |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ''), name, lttng)
  end
end

def gen_extra_event_fields_bt_model(event)
  types_name = get_extra_fields_types_name(event)
  types_name.collect do |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ''), name, lttng)
  end
end

def gen_event_bt_model(provider, c, dir)
  d = { name: "#{provider}:#{c.name}_#{SUFFIXES[dir]}" }
  m = gen_event_fields_bt_model(c, dir)

  unless m.empty?
    d[:payload_field_class] =
      {
        type: 'structure',
        members: m,
      }
  end
  d
end

def gen_extra_event_bt_model(provider, event)
  d = { name: "#{provider}:#{event['name']}" }
  m = gen_extra_event_fields_bt_model(event)

  unless m.empty?
    d[:payload_field_class] =
      {
        type: 'structure',
        members: m,
      }
  end
  d
end

def gen_yaml(event_classes, backend)
  environment =
    [
      {
        name: 'hostname',
        type: 'string',
      },
    ]

  packet_context = [
    {
      name: 'cpu_id',
      field_class: {
        type: 'integer_unsigned',
        cast_type: 'uint64_t',
        field_value_range: 32,
      },
    },
  ]

  common_context = [
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
  ]

  {
    environment: { entries: environment },
    stream_classes: [{ name: "thapi_#{backend}",
                       default_clock_class: {},
                       packet_context_field_class: { type: 'structure', members: packet_context },
                       event_common_context_field_class: { type: 'structure', members: common_context },
                       event_classes: event_classes }],
  }
end
