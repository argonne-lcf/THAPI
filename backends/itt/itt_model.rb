require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

RESULT_NAME = 'ittResult'

$itt_api_yaml = YAML.load_file('itt_api.yaml')
$itt_api = YAMLCAst.from_yaml_ast($itt_api_yaml)

typedefs = $itt_api['typedefs']
structs = $itt_api['structs']

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS = /None/

load_meta_parameters('itt_meta_parameters.yaml')

# Function we care
whitelisted_functions = %w[
  __itt_domain_create
  __itt_string_handle_create
  __itt_task_begin
  __itt_task_end
  __itt_event_create
  __itt_event_start
  __itt_event_end
  __itt_metadata_add
]

$itt_commands = $itt_api['functions'].filter_map do |func|
  next unless whitelisted_functions.include?(func.name)

  Command.new(func)
end

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ITT_POINTER_NAMES = $itt_commands.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h
