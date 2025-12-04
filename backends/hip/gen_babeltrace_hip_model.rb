require_relative 'gen_hip_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

event_classes =
  [[:lttng_ust_hip, $hip_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(provider, c, :start),
       gen_event_bt_model(provider, c, :stop)]
    end
  end.flatten(2)

hip_events = YAML.load_file(File.join(SRC_DIR, 'hip_events.yaml'))
event_classes += hip_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(provider, event)
  end
end.flatten

puts YAML.dump(gen_yaml(event_classes, 'hip'))
