require 'yaml'

INT_SIZE_MAP = {}
INT_SIGN_MAP = {}
$all_enums = {}
$all_types = {}

require_relative '../utils/gen_babeltrace_model_helper'
OPENCL_MODEL = YAML.load_file('opencl_model.yaml')

def get_bottom(type)
  type = type.gsub('cl_errcode', 'cl_int')
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
  'CL::' + type.sub(/\Acl_/, '').split('_').collect(&:capitalize).join
end

def parse_field(field)
  d = {}
  d[:field_class] = {}
  d[:name] = field['name'] if field['name']
  case field['lttng']
  when 'ctf_integer', 'ctf_integer_hex'
    d[:field_class][:type] = unsigned?(field['type']) ? 'integer_unsigned' : 'integer_signed'
    d[:field_class][:field_value_range] = integer_size(field['type'], field['pointer'])
    d[:field_class][:preferred_display_base] = 16 if field['lttng'] == 'ctf_integer_hex'
  when 'ctf_string', 'ctf_sequence_text'
    d[:field_class][:type] = 'string'
    d[:metadata] = { be_class: cl_to_class(field['type']) } if field['structure']
  when 'ctf_array'
    d[:field_class][:type] = 'array_static'
    d_field = parse_field({ 'lttng' => 'ctf_integer', 'type' => field['type'], 'pointer' => field['pointer'] })
    d[:field_class][:element_field_class] = d_field[:field_class]
    d[:field_class][:length] = field['length']
  when 'ctf_sequence'
    d[:field_class][:type] = 'array_dynamic'
    d_field = parse_field({ 'lttng' => 'ctf_integer', 'type' => field['type'], 'pointer' => field['pointer'] })
    d[:field_class][:element_field_class] = d_field[:field_class]
    d[:field_class][:length_field_path] = "EVENT_PAYLOAD[\"_#{field['name']}_length\"]"

  when 'ctf_sequence_hex'
    d[:field_class][:type] = 'array_dynamic'
    d_field = parse_field({ 'lttng' => 'ctf_integer_hex', 'type' => field['type'], 'pointer' => field['pointer'] })
    d[:field_class][:element_field_class] = d_field[:field_class]
    d[:field_class][:length_field_path] = "EVENT_PAYLOAD[\"_#{field['name']}_length\"]"
  when 'ctf_enum'
    d[:field_class][:type] = unsigned?(field['type']) ? 'enumeration_unsigned' : 'enumeration_signed'
    enum_type = field['enum_type']
    d[:field_class][:mappings] = OPENCL_MODEL['lttng_enums'][enum_type][:values].map do |f|
      { label: f[:name], integer_range_set: [[f[:value], f[:value]]] }
    end
  end
  d
end

schema_event = OPENCL_MODEL['events'].map do |name, fields|
  payload_fields = fields.map do |sub_name, field|
    field['name'] = sub_name
    parsed_field = parse_field(field)
    cast_type = "#{field['type'].gsub('cl_errcode', 'cl_int')}"
    cast_type << ' *' if field['pointer']
    cast_type << ' *' if field['array']
    cast_type << ' *' if field['string']
    cast_type << ' *' if field['structure']
    parsed_field[:field_class][:cast_type] = cast_type

    if cast_type.include?('*') && parsed_field[:field_class].include?(:element_field_class) && field['lttng'] != 'ctf_array'
      match = cast_type.match(/(.*) \*/)
      parsed_field[:field_class][:element_field_class][:cast_type] = match[1]
    end

    if (field['array'] || field['structure']) && field['lttng'].match('ctf_sequence')

      additional_parsed_field = parse_field({ 'name' => "_#{sub_name}_length",
                                              'lttng' => 'ctf_integer', 'type' => 'size_t' })
      additional_parsed_field[:field_class][:cast_type] = 'size_t'
      [additional_parsed_field, parsed_field]
    else
      parsed_field
    end
  end.flatten

  d = { name: name }
  unless payload_fields.empty?
    d[:payload_field_class ] =
      { type: 'structure',
        members: payload_fields }
  end
  d
end

puts YAML.dump(gen_yaml(schema_event, 'opencl'))
