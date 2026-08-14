require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# The Ruby bindings cover every namespace with an api.yaml; zer has none, so it
# gets no generated class.
API = APIS.values_at(:ze, :zet, :zes, :zel, :zex).inject(:+)

def to_class_name(name)
  mod = to_name_space(name)
  n = name.gsub(/_t\z/, '').gsub(/\Aze[stl]?_/, '').split('_').collect(&:capitalize).join
  mod << n.gsub('Uuid', 'UUID').gsub('Dditable', 'DDITable').gsub(/\AFp/, 'FP').gsub('P2p', 'P2P')
end

def to_scoped_class_name(name)
  "ZE::#{to_class_name(name)}"
end

def to_name_space(name)
  name.match(/\A(ze[xstlr]?)_/)[1].upcase
end

FFI_STRUCT = 'FFI::ZEStruct'
FFI_UNION = 'FFI::ZEUnion'
