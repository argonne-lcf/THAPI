require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry('itt', expect_bitfields: false)

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
