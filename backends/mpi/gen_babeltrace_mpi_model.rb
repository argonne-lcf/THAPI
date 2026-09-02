require_relative 'gen_mpi_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'mpi', expect_bitfields: false, extra_events: 'mpi_events.yaml')
