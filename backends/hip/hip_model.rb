require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'hipResult'

$hip_api = YAMLCAst.load_file('hip_api.yaml')

funcs = $hip_api['functions']
typedefs = $hip_api['typedefs']
structs = $hip_api['structs']

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS = /.*/

meta_parameters = load_meta_parameters('hip_meta_parameters.yaml')

$hip_commands = funcs.collect do |func|
  Command.new(func, meta_parameters: meta_parameters[func.name])
end

check_meta_parameters(meta_parameters, $hip_commands)

HIP_POINTER_NAMES = $hip_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
