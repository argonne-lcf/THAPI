require 'yaml'
model = ARGV[0]
namespace = ARGV[1]
babeltrace_model = YAML::load_file(model)

puts <<EOF
#pragma once
#include <babeltrace2/babeltrace.h>
EOF

# We should refractor the class_create to take a parameter
declare_signed = lambda { |_, name|
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_integer_signed_create(trace_class);
EOF
}

declare_unsigned = lambda { |_, name|
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_integer_unsigned_create(trace_class);
EOF
}

declare_string = lambda { |_, name|
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_string_create(trace_class);
EOF
}

declare_bool = lambda { |_, name|
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_bool_create(trace_class);
EOF
}

declare_structure = lambda { |_, name|
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_structure_create(trace_class);
EOF
}

declare_array_dynamic = lambda { |a, name|
  #Assume that in the model `_lenth` is provided
  #And that `_element_field_class` have been set by declare_group
  puts <<EOF
    bt_field_class *#{name}_field_class = bt_field_class_array_dynamic_create(trace_class, #{name}_element_field_class, #{name}_length_field_class);
EOF
}

def append_member(name, parent_name) 
  puts <<EOF
    bt_field_class_structure_append_member(#{parent_name}_field_class, "#{name}", #{name}_field_class);
EOF
end

default_access = lambda { |type, name|
  raise "unsupported babeltrace_type: #{type} (#{name})"
}

$print_declarators = Hash.new { |h, k| h[k] = default_access }
$print_declarators.merge!({
  'signed' => declare_signed,
  'unsigned' => declare_unsigned,
  'string' => declare_string,
  'bool' => declare_bool,
  'structure' => declare_structure,
  'array_dynamic' => declare_array_dynamic
})

def declare_group(type, method, group_name, content)

  $print_declarators["structure"].call("structure", group_name)
  content.each { |field|
    name = field[:name]
    klass = field[:class]
    if klass == "array_dynamic"
        element_klass = field[:field][:class]
        $print_declarators[element_klass].call(element_klass, "#{name}_element")
    end
    $print_declarators[klass].call(klass, name)
    append_member(name, group_name)
  }
  puts <<EOF
    #{method}(#{type}, #{group_name}_field_class);
    bt_field_class_put_ref(#{group_name}_field_class);
EOF
  content.each { |field|
    if field[:class] == "array_dynamic"
    puts <<EOF
    bt_field_class_put_ref(#{field[:name]}_element_field_class);
EOF
    end
    puts <<EOF
    bt_field_class_put_ref(#{field[:name]}_field_class);
EOF
  }
end

def declare_event_class(name, payload)
  puts <<EOF
bt_event_class* create_#{name.gsub(':','_')}_event_class_message(bt_trace_class *trace_class, bt_stream_class *stream_class) {
    bt_event_class *event_class = bt_event_class_create(stream_class);
    bt_event_class_set_name(event_class, "#{name}");
EOF
  declare_group("event_class", "bt_event_class_set_payload_field_class", "payload", payload) if payload
  puts <<EOF
    return event_class;
}
EOF
end

def declare_common_context(stream_name, common_context) 
  puts <<EOF
void populate_#{stream_name}_common_context(bt_trace_class *trace_class, bt_stream_class *stream_class) {
EOF
  declare_group("stream_class", "bt_stream_class_set_event_common_context_field_class", "common_context", common_context) if common_context
  puts <<EOF
}
EOF
end

babeltrace_model.fetch(:stream_classes,{}).each { |klass|
  declare_common_context(klass[:name], klass[:common_context])
}

babeltrace_model[:event_classes].each { |klass|
  declare_event_class(klass[:name], klass[:payload])
}
