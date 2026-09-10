require_relative '../../utils/backend_model'

API = ApiModel.load_file('hip_api.yaml').register_ffi_types

CONTEXT = BackendContext.for(API, result_name: 'hipResult', init_functions: /.*/)

COMMANDS = build_command_index(
  { lttng_ust_hip: API.functions },
  context: CONTEXT, spec: load_meta_parameters('hip_meta_parameters.yaml')
)

HIP_POINTER_NAMES = COMMANDS.pointer_names

# How each struct's byte-array members should be read.
STRUCT_SPEC = load_meta_parameters_struct('hip_meta_parameters.yaml')
