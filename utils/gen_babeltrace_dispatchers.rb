require 'yaml'
namespace = ARGV[0]
babeltrace_model = YAML::load_file("#{namespace}_babeltrace_model.yaml")

puts <<EOF
#include "babeltrace_#{namespace}.h"
#include "babeltrace_#{namespace}_callbacks.h"

EOF

print_signed = lambda { |_, name, type|
  puts <<EOF
    #{name} = (#{type})bt_field_integer_signed_get_value(_field);
EOF
}

print_unsigned = lambda { |_, name, type|
  puts <<EOF
    #{name} = (#{type})bt_field_integer_unsigned_get_value(_field);
EOF
}

print_single = lambda { |_, name, type|
  puts <<EOF
    #{name} = (#{type})bt_field_real_single_precision_get_value(_field);
EOF
}

print_double = lambda { |_, name, type|
  puts <<EOF
    #{name} = (#{type})bt_field_real_double_precision_get_value(_field);
EOF
}

print_array_access = lambda { |length, field, name, type|
  scalar_type = type.gsub("const","").sub("*", "")
  voidp = false
  if scalar_type.strip == "void"
    voidp = true
    scalar_type = "uint8_t"
  end
  puts <<EOF
    #{length} = bt_field_array_get_length(_field);
    if (#{length} > 0) {
      #{name} = (#{type})malloc(#{length}*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < #{length}; _i++) {
EOF
  case field[:field][:class]
  when 'unsigned'
    if scalar_type.match('float')
      scalar_type = 'uint32_t'
      puts <<EOF
        ((#{scalar_type} *)#{name})[_i] =
          (#{scalar_type})bt_field_integer_unsigned_get_value(
            bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    else
      puts <<EOF
        #{voidp ? "((#{scalar_type} *)#{name})" :  name}[_i] =
          (#{scalar_type})bt_field_integer_unsigned_get_value(
            bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
    end
  when 'signed'
    puts <<EOF
        #{voidp ? "((#{scalar_type} *)#{name})" :  name}[_i] =
          (#{scalar_type})bt_field_integer_signed_get_value(
            bt_field_array_borrow_element_field_by_index_const(_field, _i));
EOF
  else
    raise "unsupported array element #{field[:field][:class]}"
  end
  puts <<EOF
      }
    } else
      #{name} = NULL;
EOF
}

print_array_static = lambda { |field, name, type|
  print_array_access["_#{name}_length", field, name, type]
}

print_array_dynamic = lambda { |field, name, type|
  puts <<EOF
    size_t _sz;
EOF
  print_array_access["_sz", field, name, type]
}

print_string = lambda { |field, name, type|
    if !type.to_s.match(/\*/)
       puts <<EOF
    if (bt_field_string_get_value(_field))
      memcpy(&#{name}, bt_field_string_get_value(_field), sizeof(#{name}));
EOF
    else
      puts <<EOF
    #{name} = (#{type})bt_field_string_get_value(_field);
EOF
    end
}

default_access = lambda { |field, name, type|
    raise "unsupported babeltrace_type: #{field[:class]}"
}

$print_accessors = Hash.new { |h, k| h[k] = default_access }
$print_accessors.merge!({
  'signed' => print_signed,
  'enumeration_signed' => print_signed,
  'unsigned' => print_unsigned,
  'enumeration_unsigned' => print_unsigned,
  'single' => print_single,
  'double' => print_double,
  'array_static' => print_array_static,
  'array_dynamic' => print_array_dynamic,
  'string' => print_string
})

def print_field_member_access(f, i)
  klass = f[:class]
  name = f[:name]
  type = f[:cast_type]
  puts <<EOF
  {
    const bt_field *_field = NULL;
    _field = bt_field_structure_borrow_member_field_by_index_const(payload_field, #{i});
EOF
  $print_accessors[klass][f, name, type]
  puts <<EOF
  }
EOF
end


def print_field_members_decl(fields)
  decls = []
  fields.each { |f|
    decls.push ['size_t', "_#{f[:name]}_length"] if f[:class] == 'array_static'
    decls.push [f[:cast_type], f[:name]]
  }
  puts <<EOF unless decls.empty?
  #{decls.each.collect { |f| "#{f[0]} #{f[1]}" }.join(";\n  ")};
EOF
  decls
end

def print_field_members_access(fields)
  puts <<EOF unless fields.empty?
  const bt_field *payload_field = bt_event_borrow_payload_field_const(bt_evt);
EOF
  fields.each_with_index { |f, i|
    print_field_member_access(f, i)
  }
end

def print_field_members_free(fields)
  puts <<EOF if fields.find { |f| f[:class].match('array') }
  #{fields.each.collect { |f|
    if f[:class].match('array')
      "free((void *)#{f[:name]})"
    else
      nil
    end
  }.compact.join(";\n  ")};
EOF
end

babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  fields = klass[:payload]
  puts <<EOF
static void
#{name.gsub(":","_")}_dispatcher(
    struct #{namespace}_dispatch      *#{namespace}_dispatch,
    struct #{namespace}_callbacks     *callbacks,
    const bt_event          *bt_evt,
    const bt_clock_snapshot *bt_clock) {
EOF
decls = print_field_members_decl(fields)
print_field_members_access(fields)
puts <<EOF
  void **_p = NULL;
  while( (_p = utarray_next(callbacks->callbacks, _p)) ) {
    ((#{name.gsub(":","_")}_cb *)*_p)(
      #{(["bt_evt", "bt_clock"] + decls.collect { |f| f[1] }).join(",\n      ")});
  }
EOF
print_field_members_free(fields)
puts <<EOF
}

EOF
}

puts <<EOF
void init_dispatchers(struct #{namespace}_dispatch   *#{namespace}_dispatch) {
EOF
babeltrace_model[:event_classes].each { |klass|
  name = klass[:name]
  puts <<EOF
  #{namespace}_register_dispatcher(#{namespace}_dispatch, "#{name}", &#{name.gsub(":","_")}_dispatcher);
EOF
}
puts <<EOF
}
EOF

