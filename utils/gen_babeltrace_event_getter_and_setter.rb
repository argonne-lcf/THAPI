require 'yaml'
model = ARGV[0]
babeltrace_model = YAML::load_file(model)

puts <<EOF
#pragma once
#include <babeltrace2/babeltrace.h>
EOF

def set_scalar(field_name, index, bt2_class, name)
    puts <<EOF
     bt_field *#{name}_field = bt_field_structure_borrow_member_field_by_index(#{field_name}_field,#{index});
     bt_field_#{get_cetter(bt2_class)}_set_value(#{name}_field, #{name});
EOF
end

def set_array_dynamic(field_name, index, bt2_class, name, field)
    puts <<EOF
     bt_field *#{name}_field = bt_field_structure_borrow_member_field_by_index(#{field_name}_field,#{index});
     bt_field_array_dynamic_set_length(#{name}_field, #{name}_length);
     for (unsigned i=0; i < #{name}_length; i++) {
        bt_field *#{name}_element_field = bt_field_structure_borrow_member_field_by_index(#{name}_field, i);
        bt_field_#{get_cetter(field[:class])}_set_value(#{name}_element_field, #{name}[i]);
     }
EOF
end

def set_field(name, field, index)
    case field[:class]
    when "array_dynamic"
        set_array_dynamic(name, index, field[:class], field[:name], field[:field])
    else
        set_scalar(name, index, field[:class], field[:name])
    end
end

def get_cetter(klass)
    case klass
    when "signed"
        "integer_signed"
    when "unsigned"
        "integer_unsigned"
    else
        klass
    end
end

def get_c_type(field)
    return field[:cast_type] if field.key?(:cast_type)
    bits = field.fetch(:class_properties, {}).fetch(:field_value_range, 32)
    case field[:class]
    when "signed"
        "int#{bits}_t"
    when "unsigned"
        "uint#{bits}_t"
    when "string"
        "const char*"
    else
        field[:class]
    end
end

def create_signature_tuple(event)
    event.flat_map { |field|
          case field[:class]
          when "array_dynamic"
                [ ["#{get_c_type(field[:field])}*", field[:name] ] ]
          #        ["size_t",  "#{field[:name]}_length"] ]
          else
                [ [ get_c_type(field), field[:name] ] ]
          end
    }
end

common_context_field_setter = babeltrace_model[:stream_classes].map { |stream_class|
    name = stream_class[:name].gsub(':','_')

    l = create_signature_tuple(stream_class[:common_context])
    signature_str = l.map { |e| e.join(' ') }.join(', ')
    puts <<EOF
void bt_event_set_#{name}_common_context_fields(bt_event* event, #{signature_str}) {
EOF
    puts <<EOF
     bt_field *common_context_field = bt_event_borrow_common_context_field(event);
EOF
    stream_class[:common_context].each.with_index { |common_context, i|
        set_field("common_context", common_context, i)
    }
    puts <<EOF
}
EOF
    [name, l]
}.to_h

babeltrace_model[:event_classes].each { |event_class|
    name_stream_class = event_class.fetch("stream_class", common_context_field_setter.keys[0])
    l_common = common_context_field_setter[name_stream_class]
    l_payload = create_signature_tuple(event_class[:payload])

    l_signature_str = (l_common + l_payload).map { |e| e.join(' ') }.join(', ')

    l_common_call_str = l_common.map { |e, n| n}.join(', ')
    l_call_str = (l_common + l_payload).map { |e, n| n}.join(', ')

    puts <<EOF
void bt_event_set_#{event_class[:name].gsub(':','_')}(bt_event* event, #{l_signature_str}) {
     bt_event_set_#{name_stream_class}_common_context_fields(event, #{l_common_call_str});
     bt_field *payload_field = bt_event_borrow_payload_field(event);
EOF
    event_class[:payload].each.with_index { |payload, i|
        set_field("payload", payload, i)
    }
    puts <<EOF
}

bt_message* bt_message_create_#{event_class[:name].gsub(':','_')}(bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, #{l_signature_str}) {
     bt_message *message = bt_message_event_create(message_iterator, event_class, stream);
     bt_event *downstream_event = bt_message_event_borrow_event(message);
     bt_event_set_#{event_class[:name].gsub(':','_')}(downstream_event, #{l_call_str});
     return message;
}
EOF
}
