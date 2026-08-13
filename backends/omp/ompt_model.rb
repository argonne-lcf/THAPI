require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'ompResult'

$ompt_api = YAMLCAst.load_file('ompt_api.yaml')

typedefs = $ompt_api['typedefs']
structs = $ompt_api['structs']

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

OMPT_CALLBACKS = typedefs.select do |t|
  t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function) && t.name.match(/ompt_callback_.*_t/)
end.reject do |t|
  %w[ompt_callback_buffer_complete_t ompt_callback_buffer_request_t].include?(t.name)
end.collect do |t|
  YAMLCAst::Declaration.new(name: t.name.gsub(/_t\z/, '') + '_func', type: t.type.type)
end

INIT_FUNCTIONS = /None/

meta_parameters = load_meta_parameters('ompt_meta_parameters.yaml')

$ompt_commands = OMPT_CALLBACKS.collect do |func|
  Command.new(func, meta_parameters: meta_parameters[func.name])
end

check_meta_parameters(meta_parameters, $ompt_commands)
