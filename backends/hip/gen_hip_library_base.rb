require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# hip vendors the CUDA-compatible vector types (uchar4, dim3), GLuint/GLenum
# and activity_domain_t, none of which carry a hip prefix, so `strict` stays
# false and `name_space` can answer nil.
NAMING = NamingContext.new(
  module_name: 'HIP',
  api_files: ['hip_api.yaml'],
  namespace_pattern: /\A(hip|HIP)/
)
