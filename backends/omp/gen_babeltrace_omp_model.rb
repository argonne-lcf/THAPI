require_relative 'gen_omp_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

print_bt_model(NAMING, COMMANDS, expect_bitfields: true)
