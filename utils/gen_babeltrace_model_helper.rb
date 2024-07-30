def get_extra_fields_types_name(event)
  event["fields"].collect { |field|
    lttng = LTTng::TracepointField.new(*field)
    name = lttng.name.to_s
    type = event["args"].find { |t, n| n == name || n == name.gsub(/_val\z/, "") }[0]
    case lttng.macro.to_s
    when /ctf_sequence/
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, type, name, lttng]]
    else
      [[lttng.macro.to_s, type, name, lttng]]
    end
  }.flatten(1)
end

$types_by_name = $all_types.map { |ty| [ty.name, ty] }.to_h

def gen_bt_field_model(lttng_name, type, name, lttng)
  field = { name: name, cast_type: type}
  case lttng_name
  when 'ctf_float'
    field[:class] = type == 'float' ? 'single' : type
  when 'ctf_integer', 'ctf_integer_hex'
    field[:class] = integer_signed?(type) ? 'signed' : 'unsigned'
    field[:class_properties] = { field_value_range: integer_size(type) }
    field[:class_properties][:preferred_display_base] = 16 if lttng_name.end_with?("_hex")
    if $all_enum_names.include?(type) || $all_bitfield_names.include?(type)
      field[:be_class] = to_scoped_class_name(type)
    end
  when 'ctf_sequence', 'ctf_sequence_hex'
    array_type = lttng.type.to_s
    field[:class] = 'array_dynamic'
    field[:field] =
      { class: integer_signed?(array_type) ? 'signed' : 'unsigned',
        class_properties: { field_value_range: integer_size(array_type) } }
    field[:field][:class_properties][:preferred_display_base] = 16 if lttng_name.end_with?("_hex")
  when 'ctf_array', 'ctf_array_hex'
    array_type = lttng.type.to_s
    field[:class] = 'array_static'
    field[:field] =
      { class: integer_signed?(array_type) ? 'signed' : 'unsigned',
        class_properties: { field_value_range: integer_size(array_type) } }
    field[:field][:class_properties][:preferred_display_base] = 16 if lttng_name.end_with?("_hex")
    field[:length] = lttng.length
  when 'ctf_string'
    field[:class] = 'string'
  when 'ctf_sequence_text', 'ctf_array_text'
    field[:class] = 'string'
    field[:length] = lttng.length if lttng_name == 'ctf_array_text'
    t = type.sub(" *", "")
    while $types_by_name.include?(t) && $types_by_name[t].type.kind_of?(YAMLCAst::CustomType)
      t = $types_by_name[t].type.name
    end
    if $all_struct_names.include?(t)
      field[:be_class] = to_scoped_class_name(t)
    end
  else
    raise "unsupported lttng type: #{lttng.inspect}"
  end
  field
end

def gen_event_fields_bt_model(c, dir)
  types_name = get_fields_types_name(c, dir)
  types_name.collect { |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ""), name, lttng)
  }
end

def gen_extra_event_fields_bt_model(event)
  types_name = get_extra_fields_types_name(event)
  types_name.collect { |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ""), name, lttng)
  }
end

def gen_event_bt_model(provider, c, dir)
  { name: "#{provider}:#{c.name}_#{SUFFIXES[dir]}",
    payload: gen_event_fields_bt_model(c, dir) }
end

def gen_extra_event_bt_model(provider, event)
  { name: "#{provider}:#{event["name"]}",
    payload: gen_extra_event_fields_bt_model(event) }
end
