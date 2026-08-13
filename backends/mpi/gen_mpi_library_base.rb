require_relative 'mpi_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

API = ApiModel.new(
  types: $mpi_api['typedefs'],
  structs: $mpi_api['structs'],
  unions: $mpi_api['unions'],
  enums: $mpi_api['enums']
)

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

FFI_STRUCT = 'FFI::MPIStruct'
FFI_UNION = 'FFI::MPIUnion'
