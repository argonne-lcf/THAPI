require_relative 'mpi_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

def to_class_name(name)
  prefixed_class_name(name, to_name_space(name))
end

def to_scoped_class_name(name)
  "MPI::#{to_class_name(name)}"
end

def to_name_space(name)
  match_name_space(name, /\A(mpi|MPI)/, strict: true)
end

FFI_STRUCT = 'FFI::MPIStruct'
FFI_UNION = 'FFI::MPIUnion'
