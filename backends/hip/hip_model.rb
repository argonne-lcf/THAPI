require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('hip_api.yaml')

funcs = API.functions
typedefs = API.types
structs = API.structs

TYPE_CLASSES = find_all_types(typedefs)
gen_ffi_type_map(typedefs, TYPE_CLASSES)

CONTEXT = BackendContext.new(
  result_name: 'hipResult',
  # Every entry point initializes the tracer.
  init_functions: /.*/,
  struct_map: find_struct_map(typedefs, structs),
  type_classes: TYPE_CLASSES
)

meta_parameters = load_meta_parameters('hip_meta_parameters.yaml')

$hip_commands = funcs.collect do |func|
  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end

check_meta_parameters(meta_parameters, $hip_commands)

HIP_POINTER_NAMES = $hip_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
