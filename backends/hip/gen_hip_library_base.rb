require_relative 'hip_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $hip_api['typedefs']
$all_structs = $hip_api['structs']
$all_unions = $hip_api['unions']
$all_enums = $hip_api['enums']

$objects = find_objects($all_types, extra: ['hipGraphicsResource_t'])
$int_scalars = find_int_scalars($all_types)

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

$all_enum_names, $all_bitfield_names, $all_struct_names =
  classify_ast_types($all_types, $all_enums)

FFI_STRUCT = 'FFI::HIPStruct'
FFI_UNION = 'FFI::HIPUnion'
