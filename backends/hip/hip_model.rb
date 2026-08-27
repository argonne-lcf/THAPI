require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('hip_api.yaml')

TYPE_CLASSES = find_all_types(API.types)
gen_ffi_type_map(API.types, TYPE_CLASSES)

CONTEXT = BackendContext.new(
  result_name: 'hipResult',
  init_functions: /.*/,
  struct_map: API.struct_map,
  type_classes: TYPE_CLASSES
)

meta_parameters = load_meta_parameters('hip_meta_parameters.yaml')

COMMANDS = CommandIndex.new(lttng_ust_hip: API.functions.collect do |func|
  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end)

check_meta_parameters(meta_parameters, COMMANDS)

HIP_POINTER_NAMES = COMMANDS.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
