require_relative '../../utils/backend_model'

API = ApiModel.load_file('itt_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.new(
  result_name: 'ittResult',
  init_functions: /None/,
  struct_map: API.struct_map,
  type_classes: API.type_classes
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

COMMANDS = CommandIndex.new(lttng_ust_itt: API.functions.filter_map do |func|
  next unless whitelisted_functions.include?(func.name)

  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end)

check_meta_parameters(meta_parameters, COMMANDS)
