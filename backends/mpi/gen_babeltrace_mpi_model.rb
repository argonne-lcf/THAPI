require_relative 'gen_mpi_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = TypeRegistry.from_ast(
  all_types: $all_types, all_enums: $all_enums,
  enum_names: $all_enum_names, bitfield_names: $all_bitfield_names, struct_names: $all_struct_names,
  class_namer: method(:to_scoped_class_name),
)

event_classes =
  [[:lttng_ust_mpi, $mpi_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(registry, provider, c, :start),
       gen_event_bt_model(registry, provider, c, :stop)]
    end
  end.flatten(2)

mpi_events = YAML.load_file(File.join(SRC_DIR, 'mpi_events.yaml'))
event_classes += mpi_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(registry, provider, event)
  end
end.flatten

puts YAML.dump(gen_yaml(event_classes, 'mpi'))
