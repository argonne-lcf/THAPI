require_relative 'gen_opencl_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

# opencl_wrapper_events.yaml: the calls the tracer wraps itself (callbacks it installs).
events = %w[opencl_events.yaml opencl_wrapper_events.yaml].map { |f| File.join(SRC_DIR, f) }
print_bt_model(NAMING, COMMANDS, expect_bitfields: true, extra_events_path: events)
