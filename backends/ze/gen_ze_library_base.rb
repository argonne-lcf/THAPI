require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

API = ApiModel.new(
  types: $ze_api['typedefs'] + $zet_api['typedefs'] + $zes_api['typedefs'] + $zel_api['typedefs'] +
         $zex_api['typedefs'],
  structs: $ze_api['structs'] + $zet_api['structs'] + $zes_api['structs'] + $zel_api['structs'] +
           $zex_api['structs'],
  unions: $zet_api['unions'],
  enums: $ze_api['enums'] + $zet_api['enums'] + $zes_api['enums'] + $zel_api['enums'] +
         $zex_api['enums']
)

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
