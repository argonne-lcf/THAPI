require_relative '../../utils/backend_model'

API = ApiModel.load_file('hip_api.yaml').register_ffi_types

CONTEXT = BackendContext.for(API, result_name: 'hipResult', init_functions: /.*/)

META_PARAMETERS = load_meta_parameters('hip_meta_parameters.yaml')

COMMANDS = build_command_index(
  { lttng_ust_hip: API.functions },
  context: CONTEXT, spec: META_PARAMETERS[:meta_parameters]
)

HIP_POINTER_NAMES = COMMANDS.pointer_names
