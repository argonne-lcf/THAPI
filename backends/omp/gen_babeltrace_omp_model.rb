require_relative 'gen_omp_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = TypeRegistry.from_ast(
  all_types: $all_types, all_enums: $all_enums,
  enum_names: $all_enum_names, bitfield_names: $all_bitfield_names, struct_names: $all_struct_names,
  class_namer: method(:to_scoped_class_name),
)
raise 'omp: expected bitfield types' if registry.bitfield_names.empty?

event_classes =
  [[:lttng_ust_ompt, $ompt_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(registry, provider, c)]
    end
  end.flatten(2)

puts YAML.dump(gen_yaml(event_classes, 'omp'))
