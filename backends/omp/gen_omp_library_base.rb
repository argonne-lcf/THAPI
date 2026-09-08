require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

OMP_NAMESPACE_PATTERN = /\A(omp[dt]?)_/

NAMING = NamingContext.new(
  module_name: 'OMP',
  api: API,
  namespace_pattern: OMP_NAMESPACE_PATTERN,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    word_split_class_name(name, naming.name_space(name), OMP_NAMESPACE_PATTERN, word_case: :downcase)
  }
)
