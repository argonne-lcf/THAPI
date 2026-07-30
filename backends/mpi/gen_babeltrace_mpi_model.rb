require_relative 'gen_mpi_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

registry = build_ast_registry('mpi', expect_bitfields: false)

event_classes = gen_command_events_bt_model(registry, [[:lttng_ust_mpi, $mpi_commands]])
event_classes += gen_extra_events_bt_model(registry, 'mpi_events.yaml')

puts YAML.dump(gen_yaml(event_classes, 'mpi'))
