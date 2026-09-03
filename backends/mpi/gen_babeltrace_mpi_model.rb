require_relative 'gen_mpi_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'mpi', COMMANDS,
               expect_bitfields: false,
               extra_events_path: File.join(SRC_DIR, 'mpi_events.yaml'))
