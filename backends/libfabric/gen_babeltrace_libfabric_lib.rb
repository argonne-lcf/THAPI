require_relative '../../utils/gen_babeltrace_lib_helper'

# Passing nil for the naming argument is legal only because no field in
# btx_libfabric_model.yaml carries ":metadata: :be_class:".  If you later add
# an enum or bitmask field -- rendering fi_cq flags symbolically, say -- this
# backend will also need a libfabric_library.rb defining the value tables, and
# nil has to become that naming object.  backends/mpi is the example to copy.
add_babeltrace_event_callbacks(nil, File.join(SRC_DIR, 'btx_libfabric_model.yaml'))
