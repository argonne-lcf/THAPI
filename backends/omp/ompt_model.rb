require_relative '../../utils/backend_model'

API = ApiModel.load_file('ompt_api.yaml')

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.for(API, result_name: 'ompResult', init_functions: /None/)

OMPT_CALLBACKS = API.types.select do |t|
  t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function) && t.name.match(/ompt_callback_.*_t/)
end.reject do |t|
  %w[ompt_callback_buffer_complete_t ompt_callback_buffer_request_t].include?(t.name)
end.collect do |t|
  YAMLCAst::Declaration.new(name: t.name.gsub(/_t\z/, '') + '_func', type: t.type.type)
end

COMMANDS = build_command_index(
  { lttng_ust_ompt: OMPT_CALLBACKS },
  context: CONTEXT, spec: load_meta_parameters('ompt_meta_parameters.yaml')
)
