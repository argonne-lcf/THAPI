require_relative '../../utils/backend_model'

API = ApiModel.load_file('hip_api.yaml').register_ffi_types

CONTEXT = BackendContext.for(API, result_name: 'hipResult', init_functions: /.*/)

meta_parameters = load_meta_parameters('hip_meta_parameters.yaml')

COMMANDS = build_command_index(
  { lttng_ust_hip: API.functions },
  context: CONTEXT, spec: meta_parameters[:meta_parameters]
)

STRUCT_SPEC = meta_parameters[:meta_parameters_struct]

HIP_POINTER_NAMES = COMMANDS.pointer_names
