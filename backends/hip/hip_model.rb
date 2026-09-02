require_relative '../../utils/backend_model'

API = ApiModel.load_file('hip_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.new(
  result_name: 'hipResult',
  init_functions: /.*/,
  struct_map: API.struct_map,
  type_classes: API.type_classes
)

meta_parameters = load_meta_parameters('hip_meta_parameters.yaml')

COMMANDS = CommandIndex.new(lttng_ust_hip: API.functions.collect do |func|
  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end)

check_meta_parameters(meta_parameters, COMMANDS)

HIP_POINTER_NAMES = COMMANDS.pointer_names
