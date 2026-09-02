require_relative '../../utils/backend_model'

API = ApiModel.load_file('ompt_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.new(
  result_name: 'ompResult',
  init_functions: /None/,
  struct_map: API.struct_map,
  type_classes: API.type_classes
)

OMPT_CALLBACKS = API.types.select do |t|
  t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function) && t.name.match(/ompt_callback_.*_t/)
end.reject do |t|
  %w[ompt_callback_buffer_complete_t ompt_callback_buffer_request_t].include?(t.name)
end.collect do |t|
  YAMLCAst::Declaration.new(name: t.name.gsub(/_t\z/, '') + '_func', type: t.type.type)
end

meta_parameters = load_meta_parameters('ompt_meta_parameters.yaml')

COMMANDS = CommandIndex.new(lttng_ust_ompt: OMPT_CALLBACKS.collect do |func|
  Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
end)

check_meta_parameters(meta_parameters, COMMANDS)
