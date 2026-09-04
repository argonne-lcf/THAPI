require_relative 'mpi_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

NAMING = NamingContext.new(
  module_name: 'MPI',
  api: API,
  namespace_pattern: /\A(mpi|MPI)/,
  strict: true
)
