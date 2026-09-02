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

# cuda spells an FFI type as snake_case of its Ruby class name, namespace
# first: CUdevice -> :cu_device. cuuint32_t/cuuint64_t keep the header's own
# spelling, which that rule would mangle.
CUDA_FFI_NAMES = { 'cuuint64_t' => ':cuuint64_t', 'cuuint32_t' => ':cuuint32_t' }.freeze

FFIName.fallback = lambda { |name|
  next CUDA_FFI_NAMES[name] if CUDA_FFI_NAMES.key?(name)

  namespace = NAMING.name_space(name)
  rest = NAMING.class_name(name).sub(/\A#{namespace}/, '')
  prefix = namespace ? "#{namespace.downcase}_" : ''
  "#{prefix}#{to_snake_case(rest).gsub(/\A_+/, '')}".to_sym.inspect
}

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
