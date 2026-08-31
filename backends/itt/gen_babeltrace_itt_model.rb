require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry(NAMING, 'itt', expect_bitfields: false)

event_classes = gen_command_events_bt_model(registry, COMMANDS.groups, phased: false)
event_classes += gen_extra_events_bt_model(registry, 'itt_events.yaml')

puts YAML.dump(gen_yaml(event_classes, 'itt'))
