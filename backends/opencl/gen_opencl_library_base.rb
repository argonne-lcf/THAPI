require_relative 'opencl_model'
require_relative '../../utils/gen_library_base'

# cl_device_info -> CL::DeviceInfo, CLeglImageKHR -> CL::EglImageKHR
NAMING = NamingContext.new(
  module_name: 'CL', api: API, namespace_pattern: /\A(cl_|CL)/,
  class_namer: ->(_naming, name) { name.sub(/\A(cl_|CL)/, '').split('_').collect(&:capitalize).join }
)

# The registry writes a constant's value as a literal or a bit (`(1 << n)`).
# A constant with no value of its own is reserved, or another name for one
# that has (CL_BLOCKING is CL_TRUE).
def constant_value(str)
  return unless str

  Integer(str, exception: false) || (1 << Integer(Regexp.last_match(1)) if str.match(/\A\(1 << (\d+)\)\z/))
end
