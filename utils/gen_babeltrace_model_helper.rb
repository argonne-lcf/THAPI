# Include global variable INT_SIGN_MAP, ScalarMetaParameter, etc
require_relative 'yaml_ast'

$integer_sizes = INT_SIZE_MAP.transform_values { |v| v * 8 }
$integer_signed = INT_SIGN_MAP

$all_enums.each do |t|
  $integer_sizes["enum #{t.name}"] = 32
end

$all_enums.each do |t|
  $integer_signed["enum #{t.name}"] = true
end

def integer_size(t)
  return 64 if t.match(/\*/)
  return 64 if t.match(/\[.*\]/)

  r = $integer_sizes[t]
  raise "unknown integer type #{t}" if r.nil?

  r
end

def integer_signed?(t)
  return false if t.match(/\*/)
  return false if t.match(/\[.*\]/)

  r = $integer_signed[t]
  raise "unknown integer type #{t}" if r.nil?

  r
end

# End of global variable use
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
  member = { name: name }

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
      member[:metadata] = { be_class: to_scoped_class_name(type) }
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
    field[:type] = 'array_static'
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
    member[:metadata] = { be_class: to_scoped_class_name(t) } if $all_struct_names.include?(t)

    # Too complicated, not sure why `all_struct_names` is not enough
    unless field[:cast_type].end_with?('*')
      if $all_struct_names.include?(t) || $types_by_name[t]&.type.is_a?(YAMLCAst::Union)
        field[:cast_type_is_struct] = true
      else if type.start_with?('struct')
        field[:cast_type_is_struct] = true
      end
    end
  else
    raise "unsupported lttng type: #{lttng.inspect}"
  end
  member[:field_class] = field
  member
end

def get_fields_types_name(c, dir)
  fields = []

  r = c.type.lttng_type
  fields.push([r.macro.to_s, c.type.to_s, "#{RESULT_NAME}", r]) if dir != :start && r

  if dir != :stop
    fields += c.parameters.to_a.collect do |p|
      [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
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

def gen_event_bt_model(provider, c, dir = nil)
  d = if dir
        { name: "#{provider}:#{c.name}_#{SUFFIXES[dir]}" }
      # OMP backend
      else
        { name: "#{provider}:#{c.name.gsub(/_func\z/, '')}" }
      end

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
