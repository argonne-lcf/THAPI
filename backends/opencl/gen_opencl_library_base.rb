require_relative 'opencl_model'
require_relative '../../utils/gen_library_base'

# cl_device_info -> CL::DeviceInfo, CLeglImageKHR -> CL::EglImageKHR
NAMING = NamingContext.new(
  module_name: 'CL', api: API, namespace_pattern: /\A(cl_|CL)/,
  class_namer: ->(_naming, name) { name.sub(/\A(cl_|CL)/, '').split('_').collect(&:capitalize).join }
)
