require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

# cuda.h vendors the OpenCL and VDPAU interop typedefs (cl_event_flags,
# VdpDevice) plus cuuint32_t/cuuint64_t, which carry no CU prefix, so `strict`
# stays false and name_space can answer nil.
#
# Two types are spelled CUstream* in the header but CUStream* in Ruby, and the
# namer restores three initialisms the word split would lowercase.
NAMING = NamingContext.new(
  module_name: 'CUDA',
  api: API,
  namespace_pattern: /\A(CUDA|CU)/,
  class_namer: lambda { |naming, name|
    case name
    when 'CUstreamBatchMemOpType' then 'CUStreamBatchMemOpType'
    when 'CUstreamBatchMemOpParams' then 'CUStreamBatchMemOpParams'
    else
      mod = naming.name_space(name) || ''
      n = name.gsub(/_t\z/, '').gsub(/\A#{mod}/, '').split('_').collect do |s|
        s[0] = s[0].capitalize if s.length > 0
        s
      end.join
      mod + n.gsub('Uuid', 'UUID').gsub('Ipc', 'IPC').gsub('P2p', 'P2P')
    end
  }
)

alias original_to_ffi_name to_ffi_name

def to_ffi_name(name)
  case name
  when 'cuuint64_t'
    return ':cuuint64_t'
  when 'cuuint32_t'
    return ':cuuint32_t'
  end

  result = original_to_ffi_name(name, false)
  return result if result

  n = NAMING.class_name(name)
  mod = NAMING.name_space(name)
  if mod
    n = n.gsub(/\A#{mod}/, '')
    mod += '_'
    mod = mod.downcase
  else
    mod = ''
  end
  n = to_snake_case(n).gsub(/\A_+/, '')
  (mod + n).to_sym.inspect
end

module YAMLCAst
  class Array
    def to_ffi
      t = case type
          when Pointer
            ':pointer'
          else
            to_ffi_name(type.name)
          end
      length_ = length.is_a?(String) ? length.gsub('sizeof(CUlaunchAttributeID)', '4') : length
      [t, length_]
    end
  end
end
