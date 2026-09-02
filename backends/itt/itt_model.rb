require_relative '../../utils/backend_model'

API = ApiModel.load_file('itt_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.for(API, result_name: 'ittResult', init_functions: /None/)

# The handful of entry points itt traces, of the hundred-odd its header
# declares.
traced_functions = %w[
  __itt_domain_create
  __itt_string_handle_create
  __itt_task_begin
  __itt_task_end
  __itt_event_create
  __itt_event_start
  __itt_event_end
  __itt_metadata_add
]

COMMANDS = build_command_index(
  { lttng_ust_itt: API.functions },
  context: CONTEXT, spec: load_meta_parameters('itt_meta_parameters.yaml'),
  select: ->(func) { traced_functions.include?(func.name) }
)
