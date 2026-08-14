require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

def to_class_name(name)
  case name
  when 'CUstreamBatchMemOpType'
    return 'CUStreamBatchMemOpType'
  when 'CUstreamBatchMemOpParams'
    return 'CUStreamBatchMemOpParams'
  end
  mod = to_name_space(name)
  mod ||= ''
  n = name.gsub(/_t\z/, '').gsub(/\A#{mod}/, '').split('_').collect do |s|
    s[0] = s[0].capitalize if s.length > 0
    s
  end.join
  mod << n.gsub('Uuid', 'UUID').gsub('Ipc', 'IPC').gsub('P2p', 'P2P')
end

def to_scoped_class_name(name)
  "CUDA::#{to_class_name(name)}"
end

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

  n = to_class_name(name)
  mod = to_name_space(name)
  if mod
    n = n.gsub(/\A#{mod}/, '')
    mod << '_'
    mod.downcase!
  else
    mod = ''
  end
  n = to_snake_case(n).gsub(/\A_+/, '')
  mod << n
  mod.to_sym.inspect
end

# Not strict: cuda.h vendors the OpenCL and VDPAU interop typedefs
# (cl_event_flags, VdpDevice) plus cuuint32_t/cuuint64_t, which carry no CU
# prefix. to_ffi_name below relies on the nil.
def to_name_space(name)
  match_name_space(name, /\A(CUDA|CU)/)
end

FFI_STRUCT = 'FFI::CUDAStruct'
FFI_UNION = 'FFI::CUDAUnion'

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
