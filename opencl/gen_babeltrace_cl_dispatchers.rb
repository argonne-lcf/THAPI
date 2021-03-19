require 'yaml'

if ARGV.empty?
  namespace = "babeltrace_cl"
else
  namespace = ARGV[0]
end


opencl_babeltrace_model = YAML::load_file("opencl_babeltrace_model.yaml")

puts <<EOF
#include "#{namespace}.h"
#include "#{namespace}_callbacks.h"

EOF

def print_field_member_access(f, i)
  klass = f[:class]
  name = f[:name]
  type = f[:cast_type]
  puts <<EOF
  {
    const bt_field *_field = NULL;
    _field = bt_field_structure_borrow_member_field_by_index_const(payload_field, #{i});
EOF
  case klass
  when 'signed', 'enumeration_signed'
    puts <<EOF
    #{name} = (#{type})bt_field_integer_signed_get_value(_field);
EOF
  when 'unsigned', 'enumeration_unsigned'
    puts <<EOF
    #{name} = (#{type})bt_field_integer_unsigned_get_value(_field);
EOF
  when 'float'
    puts <<EOF
    #{name} = (#{type})bt_field_real_single_precision_get_value(_field);
EOF
  when 'double'
    puts <<EOF
    #{name} = (#{type})bt_field_real_double_precision_get_value(_field);
EOF
  when 'array_static'
    scalar_type = type.gsub("const","").sub("*", "")
    puts <<EOF
    _#{name}_length = bt_field_array_get_length(_field);
    if (_#{name}_length > 0) {
      #{name} = (#{type})malloc(_#{name}_length*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < _#{name}_length; _i++) {
EOF
    case f[:field][:class]
    when 'unsigned'
      puts <<EOF
        #{name}[_i] = (#{scalar_type})bt_field_integer_unsigned_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    when 'signed'
      puts <<EOF
        #{name}[_i] = (#{scalar_type})bt_field_integer_signed_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    else
      raise "unsupported array element #{f[:field][:class]}"
    end
    puts <<EOF
      }
    } else
      #{name} = NULL;
EOF
  when 'array_dynamic'
    scalar_type = type.gsub("const","").sub("*", "")
    puts <<EOF
    uint64_t _sz = bt_field_array_get_length(_field);
    if (_sz > 0) {
      #{name} = (#{type})malloc(_sz*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < _sz; _i++) {
EOF
    case f[:field][:class]
    when 'unsigned'
      puts <<EOF
        #{name}[_i] = (#{scalar_type})bt_field_integer_unsigned_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    when 'signed'
      puts <<EOF
        #{name}[_i] = (#{scalar_type})bt_field_integer_signed_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    else
      raise "unsupported array element #{f[:field][:class]}"
    end
    puts <<EOF
      }
    } else
      #{name} = NULL;
EOF
  when 'string'
    puts <<EOF
    #{name} = (#{type})bt_field_string_get_value(_field);
EOF
  end
  puts <<EOF
  }
EOF
end

def print_field_members_access(fields)
  puts <<EOF unless fields.empty?
  const bt_field *payload_field = bt_event_borrow_payload_field_const(bt_evt);
EOF
  fields.each_with_index { |f, i|
    print_field_member_access(f, i)
  }
end

opencl_babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  fields = klass[:payload]
  decls = []
  fields.each { |f|
    decls.push ['size_t', "_#{f[:name]}_length"] if f[:class] == 'array_static'
    decls.push [f[:cast_type], f[:name]]
  }
  puts <<EOF
static void
#{name.gsub(":","_")}_dispatcher(
    struct #{namespace}_dispatch  *dispatch,
    struct #{namespace}_callbacks *callbacks,
    const bt_event          *bt_evt,
    const bt_clock_snapshot *bt_clock) {
  #{decls.each.collect { |f| "#{f[0]} #{f[1]}" }.join(";\n  ")};
EOF
print_field_members_access(fields)
puts <<EOF
  void **_p = NULL;
  while( (_p = utarray_next(callbacks->callbacks, _p)) ) {
    ((#{namespace}_#{name.gsub(":","_")}_cb *)*_p)(
      #{(["bt_evt", "bt_clock"] + decls.collect { |f| f[1] }).join(",\n      ")});
  }
EOF
puts <<EOF if fields.find { |f| f[:class].match('array') }
  #{fields.each.collect { |f|
    if f[:class].match('array')
      "free((void *)#{f[:name]})"
    else
      nil
    end
  }.compact.join(";\n  ")};
EOF
puts <<EOF
}

EOF
}

puts <<EOF
void init_#{namespace}_dispatchers(struct #{namespace}_dispatch *dispatch) {
EOF
opencl_babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  puts <<EOF
  #{namespace}_register_dispatcher(dispatch, "#{name}", &#{name.gsub(":","_")}_dispatcher);
EOF
}
puts <<EOF
}
EOF
