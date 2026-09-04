require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, COMMANDS,
               expect_bitfields: false,
               extra_events_path: File.join(SRC_DIR, 'itt_events.yaml'))
