require_relative 'gen_hip_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry(NAMING, 'hip', expect_bitfields: false)

event_classes = gen_command_events_bt_model(registry, COMMANDS.groups)
event_classes += gen_extra_events_bt_model(registry, 'hip_events.yaml')

puts YAML.dump(gen_yaml(event_classes, 'hip'))
