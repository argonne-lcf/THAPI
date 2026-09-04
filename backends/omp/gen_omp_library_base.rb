require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

NAMING = NamingContext.new(
  module_name: 'OMP',
  api: API,
  namespace_pattern: /\A(omp[dt]?)_/,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    naming.name_space(name) +
      name.gsub(/_t\z/, '').gsub(/\Aomp[dt]?_/, '').split('_').collect(&:capitalize).join
  }
)
