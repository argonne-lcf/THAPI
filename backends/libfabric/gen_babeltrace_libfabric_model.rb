require_relative 'libfabric_model'
require_relative '../../utils/gen_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

naming = NamingContext.new(
  module_name: 'LIBFABRIC',
  api: API,
  namespace_pattern: /\Afi/
)

print_bt_model(naming, COMMANDS, expect_bitfields: false)
