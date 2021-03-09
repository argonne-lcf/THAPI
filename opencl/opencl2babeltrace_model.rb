require "yaml"

OPENCL_MODEL_LOCATION = ARGV[0]
OPENCL_MODEL = YAML::load_file(OPENCL_MODEL_LOCATION)

def is_unsigned(type)
  # object are size_t
  if OPENCL_MODEL["objects"].include?(type)
     return true
  end
  bottom = OPENCL_MODEL["type_map"].fetch(type, type)
  ["unsigned int","uintptr_t", "size_t","cl_uint","cl_ulong", "cl_ushort"].include?(bottom)
end 

def parse_field(field)
    d= {}
    # This is ugly, we should change the opencl_model format
    # to handle struct and ctf_struct properly
    d[:name] = field["name"] if field["name"] 
    case field["lttng"]
    when "ctf_integer", "ctf_integer_hex", "ctf_enum"
      d[:class] = is_unsigned(field["type"]) ? "unsigned" : "signed"
      d[:class_properties] = {preferred_display_base: 16} if field["lttng"] == "ctf_integer_hex"
    when "ctf_string", "ctf_sequence_text"
      d[:class] = "string"
    when "ctf_sequence","ctf_array"
      d[:class] = "array_dynamic"
      d[:field] = parse_field( {"lttng" => "ctf_integer", "type" => d["type"]}) 
   when "ctf_sequence_hex"
      d[:class] = "array_dynamic"
      d[:field] = parse_field( {"lttng" => "ctf_integer_hex", "type" => d["type"] })
    end

    d
end 

schema_event = OPENCL_MODEL['events'].map { | name, fields|
  payload_fields = fields.map { |name, field|
    field["name"] = name 
    parsed_field =parse_field(field)
    if field["array"] and field["lttng"].match("ctf_sequence") 
      additional_parsed_field = parse_field({"name" => "_#{name}_length", 
                                             "lttng" => "ctf_integer", "type" => "size_t"})
      [additional_parsed_field, parsed_field]
    else
      parsed_field
    end 
  }.flatten
  
  {name: name, stream: "stream0", payload: payload_fields}
}

puts YAML::dump({schema_streams: [{name: "stream0"}],
                 schema_events: schema_event })
