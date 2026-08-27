require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('cudart_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.new(
  result_name: 'cudaResult',
  # gen_cudart.rb calls _init_tracer() from every wrapper, so no function is
  # singled out as the initializer and Command#init? is never asked.
  init_functions: nil,
  struct_map: API.struct_map,
  type_classes: API.type_classes
)

COMMANDS = CommandIndex.new(lttng_ust_cudart: API.functions.collect do |func|
  Command.new(func, context: CONTEXT)
end)

CUDART_POINTER_NAMES = COMMANDS.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
