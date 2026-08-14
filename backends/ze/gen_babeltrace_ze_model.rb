require_relative 'gen_ze_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'
require 'set'

registry = build_ast_registry('ze', expect_bitfields: true)

event_classes = gen_command_events_bt_model(registry, COMMANDS.groups)
event_classes += gen_extra_events_bt_model(registry, 'ze_events.yaml')

def get_structs_types(namespace, types, structs)
  types.select do |t|
    t.type.is_a?(YAMLCAst::Struct) && (struct = structs.find do |s|
      t.type.name == s.name
    end) && struct.members.first.name == 'stype'
  end.map(&:name).reject do |n|
    n.start_with?("#{namespace}_base_")
  end.to_set
end

def gen_struct_event_bt_model(provider, struct)
  {
    name: "#{provider}:#{struct}",
    payload_field_class:
          {
            type: 'structure',
            members:
    [
      {
        name: 'p',
        field_class: {
          cast_type: "#{struct} *",
          type: 'integer_unsigned',
          field_value_range: 64,
          preferred_display_base: 16,
        },
      },
      {
        name: '_p_val_length',
        field_class: {
          cast_type: 'size_t',
          type: 'integer_unsigned',
          field_value_range: 64,
        },
      },
      {
        name: 'p_val',
        field_class: {
          cast_type: "#{struct} *",
          type: 'string',
        },
        metadata: {
          be_class: "ZE::#{to_class_name(struct)}",
        },
      },
    ],
          },
  }
end

event_classes += APIS.collect do |ns, api|
  get_structs_types(ns, api.types, api.structs).collect do |struct|
    gen_struct_event_bt_model(:"lttng_ust_#{ns}_structs", struct)
  end
end.flatten

puts YAML.dump(gen_yaml(event_classes, 'ze'))
