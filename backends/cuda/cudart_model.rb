require_relative '../../utils/backend_model'

API = ApiModel.load_file('cudart_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

# gen_cudart.rb calls _init_tracer() from every wrapper, so no function is
# singled out as the initializer and Command#init? is never asked.
CONTEXT = BackendContext.for(API, result_name: 'cudaResult', init_functions: nil)

COMMANDS = build_command_index({ lttng_ust_cudart: API.functions }, context: CONTEXT)

CUDART_POINTER_NAMES = COMMANDS.pointer_names
