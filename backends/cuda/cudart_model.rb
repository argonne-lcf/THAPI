require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('cudart_api.yaml')

funcs = API.functions
typedefs = API.types
structs = API.structs

TYPE_CLASSES = find_all_types(typedefs)
gen_ffi_type_map(typedefs, TYPE_CLASSES)

CONTEXT = BackendContext.new(
  result_name: 'cudaResult',
  # gen_cudart.rb calls _init_tracer() from every wrapper, so no function is
  # singled out as the initializer and Command#init? is never asked.
  init_functions: nil,
  struct_map: find_struct_map(typedefs, structs),
  type_classes: TYPE_CLASSES
)

COMMANDS = CommandIndex.new(lttng_ust_cudart: funcs.collect do |func|
  Command.new(func, context: CONTEXT)
end)

CUDART_POINTER_NAMES = COMMANDS.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
