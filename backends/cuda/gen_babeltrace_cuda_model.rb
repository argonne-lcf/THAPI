require_relative 'gen_cuda_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'cuda', expect_bitfields: false, extra_events: 'cuda_events.yaml')
