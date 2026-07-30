require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $ze_api['typedefs'] + $zet_api['typedefs'] + $zes_api['typedefs'] + $zel_api['typedefs'] +
             $zex_api['typedefs']
$all_structs = $ze_api['structs'] + $zet_api['structs'] + $zes_api['structs'] + $zel_api['structs'] +
               $zex_api['structs']
$all_unions = $zet_api['unions']
$all_enums = $ze_api['enums'] + $zet_api['enums'] + $zes_api['enums'] + $zel_api['enums'] +
             $zex_api['enums']
$all_funcs = $ze_api['functions'] + $zet_api['functions'] + $zes_api['functions'] + $zel_api['functions'] +
             $zex_api['functions']

$objects = $all_types.select do |t|
  t.type.is_a?(YAMLCAst::Pointer) &&
    t.type.type.is_a?(YAMLCAst::Struct)
end.collect { |t| t.name }

$all_types.each do |t|
  $objects.push t.name if t.type.is_a?(YAMLCAst::CustomType) && OBJECT_TYPES.include?(t.type.name)
end

$int_scalars = {}
$all_types.each do |t|
  $int_scalars[t.name] = t.type.name if t.type.is_a?(YAMLCAst::CustomType) && INT_TYPES.include?(t.type.name)
end

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

$all_enum_names, $all_bitfield_names, $all_struct_names, $all_union_names =
  classify_ast_types($all_types, $all_enums)

FFI_STRUCT = 'FFI::ZEStruct'
FFI_UNION = 'FFI::ZEUnion'
