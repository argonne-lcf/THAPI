require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $hip_api['typedefs']
$all_structs = $hip_api['structs']
$all_unions = $hip_api['unions']
$all_enums = $hip_api['enums']
$all_funcs = $hip_api['functions']

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

$objects = $all_types.filter_map do |t|
  t.name if t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Struct)
end
$objects.push 'hipGraphicsResource_t'

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
  mod ||= ''
  n = name.gsub(/\A#{mod}/, '')
  mod.capitalize! if mod == 'hip'
  res = mod << n
  res[0] = res[0].upcase if res[0].match(/[[:lower:]]/)
  res
end

def to_scoped_class_name(name)
  "HIP::#{to_class_name(name)}"
end

def to_name_space(name)
  case name
  when /\Ahip/
    'hip'
  when /\AHIP/
    'HIP'
  end
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    $all_enums.find { |e| t.type.name == e.name }
    $all_enum_names.push t.name
  elsif t.type.is_a? YAMLCAst::Struct
    $all_struct_names.push t.name
  elsif t.type.is_a? YAMLCAst::Union
    $all_union_names.push t.name
  end
end

FFI_STRUCT = 'FFI::HIPStruct'
FFI_UNION = 'FFI::HIPUnion'
