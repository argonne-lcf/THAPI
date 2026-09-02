require_relative 'gen_itt_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, 'itt', expect_bitfields: false, extra_events: 'itt_events.yaml', phased: false)
