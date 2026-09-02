require_relative 'gen_hip_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'hip', expect_bitfields: false, extra_events: 'hip_events.yaml')
