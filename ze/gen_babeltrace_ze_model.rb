require_relative 'gen_ze_library_base.rb'
require 'set'

$integer_sizes = {
  "uint8_t" => 8,
  "int8_t" =>  8,
  "uint16_t" => 16,
  "int16_t" => 16,
  "uint32_t" => 32,
  "int32_t" => 32,
  "uint64_t" => 64,
  "int64_t" => 64,
  "uintptr_t" => 64,
  "size_t" => 64,
  "ze_bool_t" => 8
}

ZE_ENUM_SCALARS.each { |t|
  $integer_sizes[t] = 32
}

$int_scalars.each { |t, v|
  $integer_sizes[t] = $integer_sizes[v]
}

$integer_signed = {
  "int8_t" =>  true,
  "int16_t" => true,
  "int32_t" => true,
  "int64_t" => true,
  "uint8_t" => false,
  "uint16_t" => false,
  "uint32_t" => false,
  "uint64_t" => false,
  "uintptr_t" => false,
  "size_t" => false,
  "ze_bool_t" => false
}

ZE_ENUM_SCALARS.each { |t|
  $integer_signed[t] = true
}

$int_scalars.each { |t, v|
  $integer_signed[t] = $integer_signed[v]
}

def integer_size(t)
  return 64 if t.match(/\*/)
  return 64 if $objects.include?(t)
  return 64 if $all_types_map[t].kind_of?(YAMLCAst::Pointer)
  r = $integer_sizes[t]
  raise "unknown integer type #{t}" if r.nil?
  r
end

def integer_signed?(t)
  return false if t.match(/\*/)
  return false if $objects.include?(t)
  return false if $all_types_map[t].kind_of?(YAMLCAst::Pointer)
  r = $integer_signed[t]
  raise "unknown integer type #{t}" if r.nil?
  r
end

def meta_parameter_types_name(m, dir)
  if dir == :start
    lttng = m.lttng_in_type
  else
    lttng = m.lttng_out_type
  end
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    else
      [[lttng.macro.to_s, "#{t}", "#{name}", lttng]]
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == "ctf_string"
      [["ctf_string", "#{t} *", "#{name}", lttng]]
    else
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  end
end

def get_fields_types_name(c, dir)
   if dir == :start
    fields = (c.parameters ? c.parameters : []).collect { |p|
      [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
    }
    fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
      meta_parameter_types_name(m, :start)
    }.flatten(1)
  else
    fields = [["ctf_integer", "ze_result_t", "#{RESULT_NAME}"]]
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_types_name(m, :stop)
    }.flatten(1)
  end
  fields
end

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

def gen_bt_field_model(lttng_name, type, name, lttng)
  field = { name: name, cast_type: type}
  case lttng_name
  when 'ctf_float'
    field[:class] = type == 'float' ? 'single' : type
  when 'ctf_integer', 'ctf_integer_hex'
    field[:class] = integer_signed?(type) ? 'signed' : 'unsigned'
    field[:class_properties] = { field_value_range: integer_size(type) }
    field[:class_properties][:preferred_display_base] = 16 if lttng_name.end_with?("_hex")
    t = type.sub(/_flags_t\Z/, "_flag_t")
    if $all_enum_names.include?(t) || $all_bitfield_names.include?(t)
      field[:be_class] = "ZE::#{to_class_name(t)}"
    end
  when 'ctf_sequence', 'ctf_sequence_hex'
    array_type = lttng.type.to_s
    field[:class] = 'array_dynamic'
    field[:field] =
      { class: integer_signed?(array_type) ? 'signed' : 'unsigned',
        class_properties: { field_value_range: integer_size(array_type) } }
    field[:field][:class_properties][:preferred_display_base] = 16 if lttng_name.end_with?("_hex")
  when 'ctf_array'
    array_type = lttng.type.to_s
    field[:class] = 'array_static'
    field[:field] =
      { class: integer_signed?(array_type) ? 'signed' : 'unsigned',
        class_properties: { field_value_range: integer_size(array_type) } }
    field[:length] = lttng.length
  when 'ctf_string'
    field[:class] = 'string'
  when 'ctf_sequence_text'
    field[:class] = 'string'
    if $all_struct_names.include?(type.sub(" *", ""))
      field[:be_class] = "ZE::#{to_class_name(type.sub(" *", ""))}"
    end
  when 'ctf_array_text'
    field[:class] = 'string'
    field[:length] = lttng.length
    if $all_struct_names.include?(type.sub(" *", ""))
      field[:be_class] = "ZE::#{to_class_name(type.sub(" *", ""))}"
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

event_classes = 
[[:lttng_ust_ze, $ze_commands],
 [:lttng_ust_zet, $zet_commands],
 [:lttng_ust_zes, $zes_commands],
 [:lttng_ust_zel, $zel_commands]].collect { |provider, commands|
  commands.collect { |c|
    [gen_event_bt_model(provider, c, :start),
    gen_event_bt_model(provider, c, :stop)]
  }
}.flatten(2)

def get_structs_types(namespace, types, structs)
  types.select { |t|
    t.type.kind_of?(YAMLCAst::Struct) && (struct = structs.find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
  }.map(&:name).reject { |n|
    n.start_with?("#{namespace}_base_")
  }.to_set
end

def gen_struct_event_bt_model(provider, struct)
  {
    name: "#{provider}:#{struct}",
    payload: [
      {
        name: "p",
        cast_type: "#{struct} *",
        class: "unsigned",
        class_properties: {
          field_value_range: 64,
          preferred_display_base: 16 }
      },
      {
        name: "_p_val_length",
        cast_type: "size_t",
        class: "unsigned",
        class_properties: {
          field_value_range: 64 }
      },
      {
        name: "p_val",
        cast_type: "#{struct} *",
        class: "string",
        be_class: "ZE::#{to_class_name(struct)}"
      }
    ]
  }
end

event_classes +=
[[:lttng_ust_ze_structs, get_structs_types(:ze, $ze_api["typedefs"], $ze_api["structs"])],
 [:lttng_ust_zet_structs, get_structs_types(:zet, $zet_api["typedefs"], $zet_api["structs"])],
 [:lttng_ust_zes_structs, get_structs_types(:zes, $zes_api["typedefs"], $zes_api["structs"])],
 [:lttng_ust_zel_structs, get_structs_types(:zel, $zel_api["typedefs"], $zel_api["structs"])]].collect { |provider, structs|
  structs.collect { |struct|
    gen_struct_event_bt_model(provider, struct)
  }
}.flatten

ze_events = YAML::load_file(File.join(SRC_DIR,"ze_events.yaml"))
event_classes += ze_events.collect { |provider, es|
  es["events"].collect { |event|
    gen_extra_event_bt_model(provider, event)
  }
}.flatten

puts YAML.dump({ name: "thapi_ze", event_classes: event_classes })

