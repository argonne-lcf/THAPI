require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

def to_class_name(name)
  prefixed_class_name(name, to_name_space(name))
end

def to_scoped_class_name(name)
  "HIP::#{to_class_name(name)}"
end

# Not strict: hip.h vendors the CUDA-compatible vector types (uchar4, dim3),
# GLuint/GLenum and activity_domain_t, none of which carry a hip prefix.
def to_name_space(name)
  match_name_space(name, /\A(hip|HIP)/)
end

FFI_STRUCT = 'FFI::HIPStruct'
FFI_UNION = 'FFI::HIPUnion'
