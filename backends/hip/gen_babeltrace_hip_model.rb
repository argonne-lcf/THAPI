require_relative 'gen_hip_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'hip', COMMANDS, expect_bitfields: false,
               extra_events_path: File.join(SRC_DIR, 'hip_events.yaml'))
