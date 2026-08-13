require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $ompt_api['typedefs']
$all_structs = $ompt_api['structs']
$all_unions = $ompt_api['unions']
$all_enums = $ompt_api['enums']
$all_funcs = $ompt_api['functions']

$objects = find_objects($all_types)
$int_scalars = find_int_scalars($all_types)

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

$all_enum_names, $all_bitfield_names, $all_struct_names =
  classify_ast_types($all_types, $all_enums)

FFI_STRUCT = 'FFI::OMPTStruct'
FFI_UNION = 'FFI::OMPTUnion'
