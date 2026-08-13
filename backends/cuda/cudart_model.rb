require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'cudaResult'

$cudart_api = YAMLCAst.load_file('cudart_api.yaml')

funcs = $cudart_api['functions']
typedefs = $cudart_api['typedefs']
structs = $cudart_api['structs']

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

# Currently ignored by gen_cudart.rb
INIT_FUNCTIONS = /.*/

# cudart declares no meta-parameters, so it loads no spec.
$cudart_commands = funcs.collect do |func|
  Command.new(func)
end

CUDART_POINTER_NAMES = $cudart_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
