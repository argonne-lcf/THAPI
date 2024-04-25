require 'yaml'
OPENCL_MODEL = YAML.load_file('opencl_model.yaml')

def get_bottom(type)
  type = type.gsub("cl_errcode","cl_int")
  OPENCL_MODEL['type_map'].fetch(type, type)
end

def unsigned?(type, pointer = false)
  # object are size_t
  return true if pointer || type.match(/\*/)
  return true if OPENCL_MODEL['objects'].include?(type)
  bottom = get_bottom(type)
  ['unsigned int', 'uintptr_t', 'size_t', 'cl_uint', 'cl_ulong', 'cl_ushort', 'uint64_t'].include?(bottom)
end

def integer_size(type, pointer = false)
  return 64 if pointer || type.match(/\*/)
  return 64 if OPENCL_MODEL['objects'].include?(type)
  bottom = get_bottom(type)
  case bottom
  when 'cl_char', 'cl_uchar'
    8
  when 'cl_ushort', 'cl_short'
    16
  when 'cl_int', 'cl_uint', 'unsigned int', 'int'
    32
  when 'uintptr_t', 'intptr_t', 'size_t', 'cl_long', 'cl_ulong', 'uint64_t'
    64
  else
    raise "unknown integer base type: #{bottom}"
  end
end

def cl_to_class(type)
  "CL::" + type.sub(/\Acl_/,"").split("_").collect(&:capitalize).join
end

def parse_field(field)
  d = {}
  d[:name] = field['name'] if field['name']
  case field['lttng']
  when 'ctf_integer', 'ctf_integer_hex'
    d[:class] = unsigned?(field['type']) ? 'unsigned' : 'signed'
    props = {}
    props[:field_value_range] = integer_size(field['type'], field['pointer'])
    props[:preferred_display_base] = 16 if field['lttng'] == 'ctf_integer_hex'
    d[:class_properties] = props
  when 'ctf_string', 'ctf_sequence_text'
    d[:class] = 'string'
    d[:be_class] = cl_to_class(field['type']) if field['structure']
  when 'ctf_array'
    d[:class] = 'array_static'
    d[:field] = parse_field({ 'lttng' => 'ctf_integer', 'type' => field['type'], 'pointer'=> field['pointer'] })
    d[:length] = field['length']
  when 'ctf_sequence'
    d[:class] = 'array_dynamic'
    d[:field] = parse_field({ 'lttng' => 'ctf_integer', 'type' => field['type'], 'pointer' => field['pointer'] })
  when 'ctf_sequence_hex'
    d[:class] = 'array_dynamic'
    d[:field] = parse_field({ 'lttng' => 'ctf_integer_hex', 'type' => field['type'], 'pointer' => field['pointer'] })
  when 'ctf_enum'
    d[:class] = unsigned?(field['type']) ? 'enumeration_unsigned' : 'enumeration_signed'
    enum_type = field['enum_type']
    d[:mappings] = OPENCL_MODEL['lttng_enums'][enum_type][:values].map { |f|
      { label: f[:name], integer_range_set: [[f[:value],f[:value]]] }
    }
  end
  d
end

schema_event = OPENCL_MODEL['events'].map { |name, fields|
  payload_fields = fields.map { |sub_name, field|
    field['name'] = sub_name
    parsed_field = parse_field(field)
    cast_type = "#{field['type'].gsub('cl_errcode', 'cl_int')}"
    cast_type << ' *' if field['pointer']
    cast_type << ' *' if field['array']
    cast_type << ' *' if field['string']
    cast_type << ' *' if field['structure']
    parsed_field[:cast_type] = cast_type
    if (field['array'] || field['structure']) && field['lttng'].match('ctf_sequence')
      additional_parsed_field = parse_field({ 'name' => "_#{sub_name}_length",
                                              'lttng' => 'ctf_integer', 'type' => 'size_t' })
      additional_parsed_field[:cast_type] = 'size_t'
      [additional_parsed_field, parsed_field]
    else
      parsed_field
    end
  }.flatten

  { name: name, payload: payload_fields }
}

environment = [
  {
    name: 'hostname',
    class: 'string',
  }
]

packet_context = [
  {
    name: 'cpu_id',
    class: 'unsigned',
    cast_type: 'uint64_t',
    class_properties: {
      field_value_range: 32
    }
  }
]

common_context = [
  {
    name: 'vpid',
    class: 'signed',
    cast_type: 'int64_t',
    class_properties: {
      field_value_range: 64,
    }
  },
  {
    name: 'vtid',
    class: 'unsigned',
    cast_type: 'uint64_t',
    class_properties: {
      field_value_range: 64,
    }
  }
]

puts YAML.dump({
  name: 'thapi_opencl',
  environment: environment,
  clock_snapshot_value: true,
  packet_context: packet_context,
  common_context: common_context,
  event_classes: schema_event })
