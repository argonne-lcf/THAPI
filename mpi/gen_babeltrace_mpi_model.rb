require_relative 'gen_mpi_library_base'
require_relative '../utils/gen_babeltrace_model_helper'

event_classes =
  [[:lttng_ust_mpi, $mpi_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(provider, c, :start),
       gen_event_bt_model(provider, c, :stop)]
    end
  end.flatten(2)

mpi_events = YAML.load_file(File.join(SRC_DIR, 'mpi_events.yaml'))
event_classes += mpi_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(provider, event)
  end
end.flatten

puts YAML.dump(gen_yaml(event_classes, 'mpi'))
