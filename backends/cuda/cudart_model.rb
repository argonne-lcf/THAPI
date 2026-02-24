require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'cudaResult'

$cudart_api_yaml = YAML.load_file('cudart_api.yaml')

$cudart_api = YAMLCAst.from_yaml_ast($cudart_api_yaml)

funcs = $cudart_api['functions']
typedefs = $cudart_api['typedefs']
structs = $cudart_api['structs']

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

# Currently ignored by gen_cudart.rb
INIT_FUNCTIONS = /.*/

$cudart_meta_parameters = YAML.load_file(File.join(SRC_DIR, 'cudart_meta_parameters.yaml'))
$cudart_meta_parameters['meta_parameters'].each do |func, list|
  list.each do |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  end
end

$cudart_commands = funcs.collect do |func|
  Command.new(func)
end

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

CUDART_POINTER_NAMES = $cudart_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
