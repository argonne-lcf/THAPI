require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

API = ApiModel.load_file('itt_api.yaml')

typedefs = API.types
structs = API.structs

TYPE_CLASSES = find_all_types(typedefs)
gen_ffi_type_map(typedefs, TYPE_CLASSES)

CONTEXT = BackendContext.new(
  result_name: 'ittResult',
  init_functions: /None/,
  struct_map: find_struct_map(typedefs, structs),
  type_classes: TYPE_CLASSES
)

meta_parameters = load_meta_parameters('itt_meta_parameters.yaml')

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

$itt_commands = API.functions.filter_map do |func|
  next unless whitelisted_functions.include?(func.name)

  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end

check_meta_parameters(meta_parameters, $itt_commands)
