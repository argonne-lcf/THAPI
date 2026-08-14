require_relative 'gen_cuda_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry('cuda', expect_bitfields: false)

event_classes = gen_command_events_bt_model(registry, COMMANDS.groups)
event_classes += gen_extra_events_bt_model(registry, 'cuda_events.yaml')

puts YAML.dump(gen_yaml(event_classes, 'cuda'))
