require_relative 'mpi_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $mpi_api['typedefs'] || []
$all_structs = $mpi_api['structs'] || []
$all_unions = $mpi_api['unions'] || []
$all_enums = $mpi_api['enums'] || []
$all_funcs = $mpi_api['functions'] || []

$objects = find_objects($all_types)
$int_scalars = find_int_scalars($all_types)

def to_class_name(name)
  mod = to_name_space(name)
  mod ||= ''
  n = name.gsub(/\A#{mod}/, '')
  mod.capitalize! if mod == 'mpi'
  res = mod << n
  res[0] = res[0].upcase if res[0].match(/[[:lower:]]/)
  res
end

def to_scoped_class_name(name)
  "MPI::#{to_class_name(name)}"
end

def to_name_space(name)
  case name
  when /\Ampi/
    'mpi'
  when /\AMPI/
    'MPI'
  end
end

$all_enum_names, $all_bitfield_names, $all_struct_names, $all_union_names =
  classify_ast_types($all_types, $all_enums)

FFI_STRUCT = 'FFI::MPIStruct'
FFI_UNION = 'FFI::MPIUnion'
