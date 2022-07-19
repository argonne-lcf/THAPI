require 'yaml'
model = ARGV[0]
babeltrace_model = YAML::load_file(model)

puts <<EOF
#pragma once
#include <babeltrace2/babeltrace.h>
EOF

def set_scalar(field, index, bt2_class, name)
    puts <<EOF
     bt_field *#{name}_msg_field = bt_field_structure_borrow_member_field_by_index(#{field},#{index});
     bt_field_#{bt2_class}_set_value(#{name}_msg_field, #{name});
EOF
end 


common_context_field_setter = babeltrace_model[:stream_classes].map { |stream_class|
    l = stream_class[:common_context].map { |common_context| [common_context[:class], common_context[:name] ] }
    signature_str = l.map { |e| e.join(' ') }.join(', ')
    puts <<EOF
void bt_event_set_common_context_fields(#{signature_str}) {
EOF
    puts <<EOF
     bt_field *common_context_field = bt_event_borrow_common_context_field(downstream_event);
EOF
    stream_class[:common_context].each.with_index { |common_context, i|
        set_scalar("common_context", i, common_context[:class], common_context[:name])
    }
    puts <<EOF
}
EOF
    ["default", l]
}.to_h

name = :payload
babeltrace_model[:event_classes].each { |event_class|

    l_common = common_context_field_setter["default"]
    l_payload = event_class[name].map {  |payload| [  payload[:class], payload[:name] ] }
    l_signature_str = (l_common + l_payload).map { |e| e.join(' ') }.join(', ')
    l_common_call_str = l_common.map { |e, n| n}.join(', ')
 
    puts <<EOF
void bt_event_set_#{event_class[:name].gsub(':','_')}(#{l_signature_str}) {
     bt_event_set_common_context_fields(#{l_common_call_str});
     bt_field *#{name}_field = bt_event_#{name}_field(downstream_event);
EOF
    event_class[name].each.with_index { |payload, i|
        set_scalar("#{name}_field", i, payload[:class], payload[:name]) 
    }
    puts <<EOF
}
EOF
}
