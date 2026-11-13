require_relative 'gen_opencl_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

# The calls the tracer wraps itself (callbacks it installs).
wrapper_events = lambda do |registry|
  gen_extra_events_bt_model(registry, File.join(SRC_DIR, 'opencl_wrapper_events.yaml'))
end

print_bt_model(NAMING, COMMANDS, expect_bitfields: true,
                                 extra_events_path: File.join(SRC_DIR, 'opencl_events.yaml'),
                                 extra_event_classes: wrapper_events)
