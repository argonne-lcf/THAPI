require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $ze_api['typedefs'] + $zet_api['typedefs'] + $zes_api['typedefs'] + $zel_api['typedefs']
$all_structs = $ze_api['structs'] + $zet_api['structs'] + $zes_api['structs'] + $zel_api['structs']
$all_unions = $zet_api['unions']
$all_enums = $ze_api['enums'] + $zet_api['enums'] + $zes_api['enums'] + $zel_api['enums']
$all_funcs = $ze_api['functions'] + $zet_api['functions'] + $zes_api['functions'] + $zel_api['functions']
$all_types_map = $all_types.collect { |t| [t.name, t.type] }.to_h

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

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

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if enum.members.find { |m| m.val && m.val.is_a?(String) && m.val.match('ZE_BIT') }
      $all_bitfield_names.push t.name
    else
      $all_enum_names.push t.name
    end
  elsif t.type.is_a? YAMLCAst::Struct
    $all_struct_names.push t.name
  elsif t.type.is_a? YAMLCAst::Union
    $all_union_names.push t.name
  end
end

$all_bitfield_names += $all_bitfield_names.select do |n|
  n.end_with?('_flag_t')
end.map { |n| n.gsub('_flag_t', '_flags_t') }

FFI_STRUCT = 'FFI::ZEStruct'
FFI_UNION = 'FFI::ZEUnion'
