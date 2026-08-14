require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

API = $hip_api

def to_class_name(name)
  mod = to_name_space(name)
  mod ||= ''
  n = name.gsub(/\A#{mod}/, '')
  mod.capitalize! if mod == 'hip'
  res = mod << n
  res[0] = res[0].upcase if res[0].match(/[[:lower:]]/)
  res
end

def to_scoped_class_name(name)
  "HIP::#{to_class_name(name)}"
end

def to_name_space(name)
  case name
  when /\Ahip/
    'hip'
  when /\AHIP/
    'HIP'
  end
end

FFI_STRUCT = 'FFI::HIPStruct'
FFI_UNION = 'FFI::HIPUnion'
