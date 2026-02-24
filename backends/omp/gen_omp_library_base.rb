require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $ompt_api['typedefs']
$all_structs = $ompt_api['structs']
$all_unions = $ompt_api['unions']
$all_enums = $ompt_api['enums']
$all_funcs = $ompt_api['functions']

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

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

def to_class_name(name)
  mod = to_name_space(name)
  n = name.gsub(/_t\z/, '').gsub(/\Aomp[dt]?_/, '').split('_').collect(&:capitalize).join
  mod << n
end

def to_scoped_class_name(name)
  "OMP::#{to_class_name(name)}"
end

def to_name_space(name)
  name.match(/\A(omp[dt]?)_/)[1].upcase
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if enum.name.end_with?('flag_t')
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

FFI_STRUCT = 'FFI::OMPTStruct'
FFI_UNION = 'FFI::OMPTUnion'
