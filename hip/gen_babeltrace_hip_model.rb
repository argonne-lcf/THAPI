require_relative 'gen_hip_library_base'
require_relative '../utils/gen_babeltrace_model_helper'

def get_fields_types_name(c, dir)
  if dir == :start
    fields = (c.parameters || []).collect do |p|
      [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
    end
    fields += c.meta_parameters.select { |m| m.is_a?(In) }.collect do |m|
      meta_parameter_types_name(m, :start)
    end.flatten(1)
  else
    r = c.type.lttng_type
    fields = if r
               [[r.macro.to_s, c.type.to_s, "#{RESULT_NAME}", r]]
             else
               []
             end
    fields += c.meta_parameters.select { |m| m.is_a?(Out) }.collect do |m|
      meta_parameter_types_name(m, :stop)
    end.flatten(1)
  end
  fields
end

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
