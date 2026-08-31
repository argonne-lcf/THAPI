require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# OMPT names its FFI base classes OMPT* while the Ruby module is OMP, so the
# two cannot be derived from one another.
NAMING = NamingContext.new(
  module_name: 'OMP',
  ffi_prefix: 'OMPT',
  api_files: ['ompt_api.yaml'],
  namespace_pattern: /\A(omp[dt]?)_/,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    naming.name_space(name) +
      name.gsub(/_t\z/, '').gsub(/\Aomp[dt]?_/, '').split('_').collect(&:capitalize).join
  }
)
