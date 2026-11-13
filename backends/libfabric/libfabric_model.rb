require_relative '../../utils/backend_model'

API = ApiModel.load_file('libfabric_api.yaml').register_ffi_types # to determine the size and signedness of typedefs such as fi_addr_t
api = API

# hooks = {
#   'fi_send' => ['fi_ops_msg', 'send']
# }
hooks = {
  'fi_send'    => ['fi_ops_msg', 'send'],
  'fi_recv'    => ['fi_ops_msg', 'recv'],
  'fi_cq_read' => ['fi_ops_cq', 'read']
}

functions = hooks.map do |name, (table_name, member_name)|
  table = api.struct_named(table_name)
  abort "Missing #{table_name}" unless table

  member = table.members.find { |m| m.name == member_name }
  abort "Missing #{table_name}.#{member_name}" unless member

  YAMLCAst::Declaration.new(name: name, type: member.type.type)
end

context = BackendContext.for(
  api, result_name: 'ret', init_functions: /(?!)/
)

COMMANDS = build_command_index(
  { lttng_ust_libfabric: functions }, context: context
)