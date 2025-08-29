require_relative 'gen_ze_library_base'
require_relative '../utils/gen_babeltrace_model_helper'
require 'set'

$integer_sizes = INT_SIZE_MAP.transform_values { |v| v * 8 }
$integer_signed = INT_SIGN_MAP

def integer_size(t)
  return 64 if t.match(/\*/)

  r = $integer_sizes[t]
  raise "unknown integer type #{t}" if r.nil?

  r
end

def integer_signed?(t)
  return false if t.match(/\*/)

  r = $integer_signed[t]
  raise "unknown integer type #{t}" if r.nil?

  r
end

def meta_parameter_types_name(m, dir)
  lttng = if dir == :start
            m.lttng_in_type
          else
            m.lttng_out_type
          end
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    else
      [[lttng.macro.to_s, "#{t}", "#{name}", lttng]]
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == 'ctf_string'
      [['ctf_string', "#{t} *", "#{name}", lttng]]
    else
      [['ctf_integer', 'size_t', "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  when OutPtrString
    [['ctf_string', "#{t}", "#{name}", lttng]]
  else
    raise "unsupported meta parameter class #{m.class} #{lttng.call_string} #{t}"
  end
end

def get_fields_types_name(c, dir)
  if dir == :start
    fields = (c.parameters || []).collect do |p|
      [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
    end
    fields += c.meta_parameters.select { |m| m.is_a?(In) }.collect do |m|
      meta_parameter_types_name(m, :start)
    end.flatten(1)
  else
    r = c.type.lttng_type
    fields = []
    fields = [[r.macro.to_s, c.type.to_s, "#{RESULT_NAME}", r]] if r
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_types_name(m, :stop)
    end.flatten(1)
  end
  fields
end

event_classes =
  [[:lttng_ust_ze, $ze_commands],
   [:lttng_ust_zet, $zet_commands],
   [:lttng_ust_zes, $zes_commands],
   [:lttng_ust_zel, $zel_commands],
   [:lttng_ust_zex, $zex_commands]].collect do |provider, commands|
    commands.collect do |c|
      [gen_event_bt_model(provider, c, :start),
       gen_event_bt_model(provider, c, :stop)]
    end
  end.flatten(2)

ze_events = YAML.load_file(File.join(SRC_DIR, 'ze_events.yaml'))
event_classes += ze_events.collect do |provider, es|
  es['events'].collect do |event|
    gen_extra_event_bt_model(provider, event)
  end
end.flatten

def get_structs_types(namespace, types, structs)
  types.select do |t|
    t.type.is_a?(YAMLCAst::Struct) && (struct = structs.find do |s|
      t.type.name == s.name
    end) && struct.members.first.name == 'stype'
  end.map(&:name).reject do |n|
    n.start_with?("#{namespace}_base_")
  end.to_set
end

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
          be_class: "ZE::#{to_class_name(struct)}",
        },
      },
    ],
          },
  }
end

event_classes +=
  [[:lttng_ust_ze_structs, get_structs_types(:ze, $ze_api['typedefs'], $ze_api['structs'])],
   [:lttng_ust_zet_structs, get_structs_types(:zet, $zet_api['typedefs'], $zet_api['structs'])],
   [:lttng_ust_zes_structs, get_structs_types(:zes, $zes_api['typedefs'], $zes_api['structs'])],
   [:lttng_ust_zel_structs, get_structs_types(:zel, $zel_api['typedefs'], $zel_api['structs'])],
   [:lttng_ust_zex_structs,
    get_structs_types(:zex, $zes_api['typedefs'], $zes_api['structs'])]].collect do |provider, structs|
    structs.collect do |struct|
      gen_struct_event_bt_model(provider, struct)
    end
  end.flatten

environment =
  [
    {
      name: 'hostname',
      type: 'string',
    },
  ]

packet_context = [
  {
    name: 'cpu_id',
    field_class: {
      type: 'integer_unsigned',
      cast_type: 'uint64_t',
      field_value_range: 32,
    },
  },
]

common_context = [
  {
    name: 'vpid',
    field_class: {
      type: 'integer_signed',
      cast_type: 'int64_t',
      field_value_range: 64,
    },
  },
  {
    name: 'vtid',
    field_class: {
      type: 'integer_unsigned',
      cast_type: 'uint64_t',
      field_value_range: 64,
    },
  },
]

puts YAML.dump({
                 environment: { entries: environment },
                 stream_classes: [{ name: 'thapi_ze',
                                    default_clock_class: {},
                                    packet_context_field_class: { type: 'structure', members: packet_context },
                                    event_common_context_field_class: { type: 'structure', members: common_context },
                                    event_classes: event_classes }],
               })
