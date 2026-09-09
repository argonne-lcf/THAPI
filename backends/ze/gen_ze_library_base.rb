require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# Regex for finding the namespace prefix: zet_metric_properties_t -> zet_
# No zer_, the raytracing types are spelled ze_rtas_*.
ZE_NAMESPACE_PATTERN = /\A(ze[xstl]?)_/

# The initialisms the word split would otherwise lower-case.
ZE_INITIALISMS = { 'Uuid' => 'UUID', 'Dditable' => 'DDITable',
                   /\AFp/ => 'FP', 'P2p' => 'P2P' }.freeze

NAMING = NamingContext.new(
  module_name: 'ZE',
  api: API,
  namespace_pattern: ZE_NAMESPACE_PATTERN,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    word_split_class_name(name, naming.name_space(name), ZE_NAMESPACE_PATTERN,
                          word_case: :downcase, initialisms: ZE_INITIALISMS)
  }
)
