require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('mpi_api.yaml')

TYPE_CLASSES = find_all_types(API.types)
gen_ffi_type_map(API.types, TYPE_CLASSES)

init_functions = /
  \b(?:P?MPI_Init|
  P?MPI_Init_thread|
  P?MPI_Initialized|
  P?MPI_Finalized|
  P?MPI_Get_version|
  P?MPI_Get_library_version|
  P?MPI_Info_create|
  P?MPI_Info_create_env|
  P?MPI_Info_set|
  P?MPI_Info_delete|
  P?MPI_Info_get_string|
  P?MPI_Info_get_nkeys|
  P?MPI_Info_get_nthkey|
  P?MPI_Info_dup|
  P?MPI_Info_free|
  P?MPI_Info_f2c|
  P?MPI_Info_c2f|
  P?MPI_Session_create_errhandler|
  P?MPI_Session_call_errhandler|
  P?MPI_Errhandler_free|
  P?MPI_Errhandler_f2c|
  P?MPI_Errhandler_c2f|
  P?MPI_Error_string|
  P?MPI_Error_class|
  P?MPI_Add_error_class|
  P?MPI_Remove_error_class|
  P?MPI_Add_error_code|
  P?MPI_Remove_error_code|
  P?MPI_Add_error_string|
  P?MPI_Remove_error_string|
  P?MPI_T_init_thread)\b
/ix

CONTEXT = BackendContext.new(
  result_name: 'mpiResult',
  init_functions: init_functions,
  struct_map: API.struct_map,
  type_classes: TYPE_CLASSES
)

meta_parameters = load_meta_parameters('mpi_meta_parameters.yaml')

COMMANDS = CommandIndex.new(lttng_ust_mpi: API.functions.collect do |func|
  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end)

check_meta_parameters(meta_parameters, COMMANDS)

# MPI spells its functions MPI_Comm_rank, already snake_case, so the macro name
# is the function name upcased. The other backends snake-case a camelCase name
# with upper_snake_case; here that would give _MPI__COMM_RANK_PTR.
MPI_POINTER_NAMES = COMMANDS.collect do |c|
  [c, c.pointer_name.upcase]
end.to_h

COMMANDS.add_epilogue 'MPI_Type_commit', <<EOF
  int size;#{' '}
  MPI_TYPE_SIZE_PTR(*datatype, &size);
  if (tracepoint_enabled(lttng_ust_mpi_type, property))#{' '}
    tracepoint(lttng_ust_mpi_type, property, *datatype, size);
EOF
