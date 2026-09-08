require_relative 'cuda_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# cuda.h vendors the OpenCL and VDPAU interop typedefs (cl_event_flags,
# VdpDevice) plus cuuint32_t/cuuint64_t, which carry no CU prefix, so `strict`
# stays false and name_space can answer nil.
#
# Two types are spelled CUstream* in the header but CUStream* in Ruby, and the
# namer restores three initialisms the word split would lowercase.
CUDA_NAMESPACE_PATTERN = /\A(CUDA|CU)/

NAMING = NamingContext.new(
  module_name: 'CUDA',
  api: API,
  namespace_pattern: CUDA_NAMESPACE_PATTERN,
  class_namer: lambda { |naming, name|
    word_split_class_name(name, naming.name_space(name), CUDA_NAMESPACE_PATTERN,
                          initialisms: { 'Uuid' => 'UUID', 'Ipc' => 'IPC', 'P2p' => 'P2P' })
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
  :"#{prefix}#{lower_snake_case(rest).gsub(/\A_+/, '')}".inspect
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
      length_ = if length.is_a?(String)
                  length.gsub('sizeof(CUlaunchAttributeID)', '4')
                        .gsub('sizeof(CUcheckpointGpuPair*)', '8')
                else
                  length
                end
      [t, length_]
    end
  end
end
