require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'mpiResult'

$mpi_api_yaml = YAML.load_file('mpi_api.yaml')
$mpi_api = YAMLCAst.from_yaml_ast($mpi_api_yaml)

typedefs = $mpi_api.fetch('typedefs', [])
structs = $mpi_api.fetch('structs', [])

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

mpi_funcs_e = $mpi_api['functions']

INIT_FUNCTIONS = /
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

$mpi_meta_parameters = YAML.load_file(File.join(SRC_DIR, 'mpi_meta_parameters.yaml'))
$mpi_meta_parameters.fetch('meta_parameters', []).each do |func, list|
  list.each do |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  end
end

$mpi_commands = mpi_funcs_e.collect do |func|
  Command.new(func)
end

# https://api.rubyonrails.org/classes/ActiveSupport/Inflector.html#method-i-underscore
# As a rule of thumb you can think of underscore as the inverse of camelize,
def underscore(camel_cased_word)
  return camel_cased_word.to_s.dup unless /[A-Z-]|::/.match?(camel_cased_word)

  word = camel_cased_word.to_s.gsub('::', '/')
  word.gsub!(/(?=a)b/) { "#{Regexp.last_match(1) && '_'}#{Regexp.last_match(2).downcase}" }
  word.gsub!(/(?<=[A-Z])(?=[A-Z][a-z])|(?<=[a-z\d])(?=[A-Z])/, '_')
  word.tr!('-', '_')
  word.downcase!
  word
end

MPI_POINTER_NAMES = $mpi_commands.collect do |c|
  [c, underscore(c.pointer_name).upcase]
end.to_h

register_epilogue 'MPI_Type_commit', <<EOF
  int size;#{' '}
  MPI_TYPE_SIZE_PTR(*datatype, &size);
  if (tracepoint_enabled(lttng_ust_mpi_type, property))#{' '}
    tracepoint(lttng_ust_mpi_type, property, *datatype, size);
EOF
