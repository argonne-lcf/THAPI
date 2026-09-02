require_relative 'gen_ze_library_base'
require_relative '../../utils/gen_babeltrace_model_helper'

def gen_struct_event_bt_model(provider, struct)
  {
    name: "#{provider}:#{struct}",
    payload_field_class:
          {
            type: 'structure',
            members:
    [
      {
        name: 'p',
        field_class: {
          cast_type: "#{struct} *",
          type: 'integer_unsigned',
          field_value_range: 64,
          preferred_display_base: 16,
        },
      },
      {
        name: '_p_val_length',
        field_class: {
          cast_type: 'size_t',
          type: 'integer_unsigned',
          field_value_range: 64,
        },
      },
      {
        name: 'p_val',
        field_class: {
          cast_type: "#{struct} *",
          type: 'string',
        },
        metadata: {
          be_class: "ZE::#{NAMING.class_name(struct)}",
        },
      },
    ],
          },
  }
end

# Each self-describing struct is traced as an event of its own, carrying the
# struct's bytes; no other backend has these.
def struct_event_classes
  APIS.collect do |ns, api|
    concrete_tagged_structs(ns, api).collect do |struct|
      gen_struct_event_bt_model(:"lttng_ust_#{ns}_structs", struct)
    end
  end.flatten
end

print_bt_model(NAMING, 'ze', expect_bitfields: true, extra_events: 'ze_events.yaml',
               extra_event_classes: struct_event_classes)
