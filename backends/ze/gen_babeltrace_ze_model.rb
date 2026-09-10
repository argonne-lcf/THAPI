require_relative 'gen_ze_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

# A length this side never reads: gen_bt_field_model's ctf_sequence_text branch
# takes the count from the companion length field instead.
UNREAD_LENGTH = '0'

# These are the rows print_struct_tracepoint emits into the provider, so both
# sides of the wire describe the same fields.
def gen_struct_event_bt_model(registry, provider, struct)
  address = LTTng::TracepointField.new('ctf_integer_hex', 'uintptr_t', 'p', 'p')
  bytes = LTTng::TracepointField.new('ctf_sequence_text', 'uint8_t', 'p_val', 'p', 'size_t', UNREAD_LENGTH)

  gen_bt_event(registry, provider, struct,
               [['ctf_integer_hex', "#{struct} *", 'p', address],
                *field_types_name('ctf_sequence_text', "#{struct} *", 'p_val', bytes)])
end

# Each self-describing struct is traced as an event of its own, carrying the
# struct's bytes; no other backend has these.
def struct_event_classes(registry)
  APIS.collect do |ns, api|
    traced_structs(api).collect do |struct|
      gen_struct_event_bt_model(registry, :"lttng_ust_#{ns}_structs", struct)
    end
  end.flatten
end

print_bt_model(NAMING, COMMANDS,
               expect_bitfields: true,
               extra_events_path: File.join(SRC_DIR, 'ze_events.yaml'),
               extra_event_classes: method(:struct_event_classes))
