require_relative 'opencl_model'
require_relative '../../utils/gen_probe_base'

print_tracepoint_provider(:lttng_ust_opencl, COMMANDS, include: <<~INC)
  #define CL_TARGET_OPENCL_VERSION 300
  #include <CL/opencl.h>
  #include <CL/cl_ext.h>
  #include <CL/cl_gl.h>
  #include <CL/cl_egl.h>
  #include "./tracer_opencl.h"
INC

# The calls the tracer wraps itself (callbacks it installs), after the API's own.
print_declared_events(:lttng_ust_opencl,
                      declared_events(File.join(SRC_DIR, 'opencl_wrapper_events.yaml'), :lttng_ust_opencl))
