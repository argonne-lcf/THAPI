require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# The Ruby bindings cover every namespace with an api.yaml; zer has none, so it
# gets no generated class. This is a subset of API, which is everything the
# tracer wraps.
BOUND_API = APIS.values_at(:ze, :zet, :zes, :zel, :zex).reduce(:+)

# ze headers are camelCase, so the shared prefix rule is not enough: every word
# is recased, and four initialisms are restored afterwards.
NAMING = NamingContext.new(
  module_name: 'ZE',
  api: API,
  namespace_pattern: /\A(ze[xstlr]?)_/,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    n = name.gsub(/_t\z/, '').gsub(/\Aze[stl]?_/, '').split('_').collect(&:capitalize).join
    naming.name_space(name) +
      n.gsub('Uuid', 'UUID').gsub('Dditable', 'DDITable').gsub(/\AFp/, 'FP').gsub('P2p', 'P2P')
  }
)
