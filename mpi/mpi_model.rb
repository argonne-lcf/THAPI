require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast_lttng'
require_relative '../utils/LTTng'
require_relative '../utils/command'
require_relative '../utils/meta_parameters'

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

INIT_FUNCTIONS = /MPI_Init|MPI_Init_thread/

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
