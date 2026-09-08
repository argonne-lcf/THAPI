require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# Which namespaces ze traces. zex and zer are absent from the class-name strip
# below on purpose: their typedefs keep the namespace in the class name
# (zex_wait_on_mem_desc_t -> ZEXZexWaitOnMemDesc), which is what the generated
# bindings already spell.
ZE_NAMESPACE_PATTERN = /\A(ze[xstlr]?)_/
ZE_CLASS_STRIP_PATTERN = /\Aze[stl]?_/

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
    word_split_class_name(name, naming.name_space(name), ZE_CLASS_STRIP_PATTERN,
                          word_case: :downcase, initialisms: ZE_INITIALISMS)
  }
)
