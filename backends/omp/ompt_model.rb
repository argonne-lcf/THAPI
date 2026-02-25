require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'ompResult'

$ompt_api_yaml = YAML.load_file('ompt_api.yaml')
$ompt_api = YAMLCAst.from_yaml_ast($ompt_api_yaml)

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

$ompt_meta_parameters = YAML.load_file(File.join(SRC_DIR, 'ompt_meta_parameters.yaml'))
$ompt_meta_parameters['meta_parameters'].each do |func, list|
  list.each do |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  end
end

$ompt_commands = OMPT_CALLBACKS.collect do |func|
  Command.new(func)
end

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

OMPT_POINTER_NAMES = $ompt_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
