require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = TypeRegistry.from_ast(
  all_types: $all_types, all_enums: $all_enums,
  enum_names: $all_enum_names, bitfield_names: $all_bitfield_names, struct_names: $all_struct_names,
  class_namer: method(:to_scoped_class_name),
)

event_classes =
  [[:lttng_ust_itt, $itt_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(registry, provider, c)]
    end
  end.flatten(2)

itt_events = YAML.load_file(File.join(SRC_DIR, 'itt_events.yaml'))
event_classes += itt_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(registry, provider, event)
  end
end.flatten

puts YAML.dump(gen_yaml(event_classes, 'itt'))
