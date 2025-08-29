require_relative 'gen_ze_library_base'
require_relative '../utils/gen_babeltrace_model_helper'
require 'set'

event_classes =
  [[:lttng_ust_ze, $ze_commands],
   [:lttng_ust_zet, $zet_commands],
   [:lttng_ust_zes, $zes_commands],
   [:lttng_ust_zel, $zel_commands],
   [:lttng_ust_zex, $zex_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(provider, c, :start),
       gen_event_bt_model(provider, c, :stop)]
    end
  end.flatten(2)

ze_events = YAML.load_file(File.join(SRC_DIR, 'ze_events.yaml'))
event_classes += ze_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(provider, event)
  end
end.flatten

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

event_classes +=
  [[:lttng_ust_ze_structs, get_structs_types(:ze, $ze_api['typedefs'], $ze_api['structs'])],
   [:lttng_ust_zet_structs, get_structs_types(:zet, $zet_api['typedefs'], $zet_api['structs'])],
   [:lttng_ust_zes_structs, get_structs_types(:zes, $zes_api['typedefs'], $zes_api['structs'])],
   [:lttng_ust_zel_structs, get_structs_types(:zel, $zel_api['typedefs'], $zel_api['structs'])],
   [:lttng_ust_zex_structs,
    get_structs_types(:zex, $zes_api['typedefs'], $zes_api['structs'])]].collect do |provider, structs|
    structs.collect do |struct|
      gen_struct_event_bt_model(provider, struct)
    end
  end.flatten

puts YAML.dump(gen_yaml(event_classes, 'ze'))
