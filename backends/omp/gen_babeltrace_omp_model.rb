require_relative 'gen_omp_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry(NAMING, 'omp', expect_bitfields: true)

event_classes = gen_command_events_bt_model(registry, COMMANDS.groups, phased: false)

puts YAML.dump(gen_yaml(event_classes, 'omp'))
