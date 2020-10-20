require 'yaml'
opencl_model = YAML::load_file("opencl_model.yaml")

puts <<EOF
#include "babeltrace_cl.h"
#include "babeltrace_cl_callbacks.h"

EOF

def print_field_member_access(lttng, type, name, i)
  puts <<EOF
  {
    const bt_field *_field = NULL;
    _field = bt_field_structure_borrow_member_field_by_index_const(payload_field, #{i});
EOF
  case lttng
  when "ctf_integer", "ctf_integer_hex", "ctf_enum"
    puts <<EOF
    bt_field_class_type _type = bt_field_get_class_type(_field);
    if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER))
      #{name} = (#{type})bt_field_integer_unsigned_get_value(_field);
    else
      #{name} = (#{type})bt_field_integer_signed_get_value(_field);
EOF
  when "ctf_array"
    scalar_type = type.gsub("const","").sub("*", "")
    puts <<EOF
    _#{name}_length = bt_field_array_get_length(_field);
    if (_#{name}_length > 0) {
      bt_field_class_type _type = bt_field_get_class_type(bt_field_array_borrow_element_field_by_index_const(_field, 0));
      #{name} = (#{type})malloc(_#{name}_length*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < _#{name}_length; _i++) {
        if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER))
          #{name}[_i] = (#{scalar_type})bt_field_integer_unsigned_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
        else
          #{name}[_i] = (#{scalar_type})bt_field_integer_signed_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
      }
    } else
      #{name} = NULL;
EOF
  when "ctf_sequence", "ctf_sequence_hex"
    scalar_type = type.gsub("const","").sub("*", "")
    puts <<EOF
    uint64_t _sz = bt_field_array_get_length(_field);
    if (_sz > 0) {
      bt_field_class_type _type = bt_field_get_class_type(bt_field_array_borrow_element_field_by_index_const(_field, 0));
      #{name} = (#{type})malloc(_sz*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < _sz; _i++) {
        if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER))
          #{name}[_i] = (#{scalar_type})bt_field_integer_unsigned_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
        else
          #{name}[_i] = (#{scalar_type})bt_field_integer_signed_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
      }
    } else
      #{name} = NULL;
EOF
  when "ctf_sequence_text", "ctf_string"
    puts <<EOF
    #{name} = (#{type})bt_field_string_get_value(_field);
EOF
  end
  puts <<EOF
  }
EOF
end

def print_field_members_access(fields)
  arr = []
  fields.each.collect { |n, f|
    lttng = f["lttng"]
    type = "#{f["type"].gsub("cl_errcode", "cl_int")}"
    type << " *"  if f["pointer"]
    type << " *"  if f["array"]
    type << " *"  if f["string"]
    name = n
    if f["array"] and f["lttng"].match("ctf_sequence")
      arr.push ["ctf_integer", "size_t", "_#{n}_length"]
    end
    arr.push [lttng, type, name]
  }
  puts <<EOF unless arr.empty?
  const bt_field *payload_field = bt_event_borrow_payload_field_const(bt_evt);
EOF
  arr.each_with_index { |(lttng, type, name), i|
    print_field_member_access(lttng, type, name, i)
  }
end

opencl_model["events"].each { |name, fields|
  puts <<EOF
static void
#{name.gsub(":","_")}_dispatcher(
    struct opencl_dispatch  *opencl_dispatch,
    struct opencl_callbacks *callbacks,
    const bt_event          *bt_evt,
    const bt_clock_snapshot *bt_clock) {
  #{fields.each.collect { |n, f|
  s =  "#{f["type"].gsub("cl_errcode", "cl_int")}"
  s << " *" if f["pointer"]
  s << " *" if f["array"]
  s << " *" if f["string"]
  s << " #{n}"
  if f["array"]
    ["size_t _#{n}_length", s]
  else
    s
  end
}.flatten.join(";\n  ")};
EOF
print_field_members_access(fields)
puts <<EOF
  void **p = NULL;
  while( (p = utarray_next(callbacks->callbacks, p)) ) {
    ((#{name.gsub(":","_")}_cb *)*p)(
      #{(["bt_evt", "bt_clock"] + fields.each.collect { |n, f|
        s = "#{n}"
        if f["array"]
          ["_#{n}_length", s]
        else
          s
        end
      }).flatten.join(",\n      ")});
  }
  #{fields.each.collect { |n, f|
    if f["array"] && f["lttng"] != "ctf_sequence_text"
      "if (#{n}) free((void *)#{n})"
    else
      nil
    end
  }.compact.join(";\n  ")};
}

EOF
}

puts <<EOF
void init_dispatchers(struct opencl_dispatch   *opencl_dispatch) {
EOF
opencl_model["events"].each_key { |name|
  puts <<EOF
  opencl_register_dispatcher(opencl_dispatch, "#{name}", &#{name.gsub(":","_")}_dispatcher);
EOF
}
puts <<EOF
}
EOF
