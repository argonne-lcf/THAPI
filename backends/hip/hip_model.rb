require_relative '../../utils/backend_model'

API = ApiModel.load_file('hip_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.for(API, result_name: 'hipResult', init_functions: /.*/)

COMMANDS = build_command_index(
  { lttng_ust_hip: API.functions },
  context: CONTEXT, spec: load_meta_parameters('hip_meta_parameters.yaml')
)

HIP_POINTER_NAMES = COMMANDS.pointer_names
