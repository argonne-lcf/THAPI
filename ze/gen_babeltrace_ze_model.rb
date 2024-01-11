require_relative 'gen_ze_library_base.rb'
require_relative '../utils/gen_babeltrace_model_helper'
require 'set'

$integer_sizes = INT_SIZE_MAP.transform_values { |v| v*8 }
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
  if dir == :start
    lttng = m.lttng_in_type
  else
    lttng = m.lttng_out_type
  end
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    else
      [[lttng.macro.to_s, "#{t}", "#{name}", lttng]]
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == "ctf_string"
      [["ctf_string", "#{t} *", "#{name}", lttng]]
    else
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  when OutPtrString
    [["ctf_string", "#{t}", "#{name}", lttng]]
  else
    raise "unsupported meta parameter class #{m.class} #{lttng.call_string} #{t}"
  end
end

def get_fields_types_name(c, dir)
   if dir == :start
    fields = (c.parameters ? c.parameters : []).collect { |p|
      [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
    }
    fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
      meta_parameter_types_name(m, :start)
    }.flatten(1)
  else
    fields = [["ctf_integer", "ze_result_t", "#{RESULT_NAME}"]]
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_types_name(m, :stop)
    }.flatten(1)
  end
  fields
end

event_classes = 
[[:lttng_ust_ze, $ze_commands],
 [:lttng_ust_zet, $zet_commands],
 [:lttng_ust_zes, $zes_commands],
 [:lttng_ust_zel, $zel_commands]].collect { |provider, commands|
  commands.collect { |c|
    [gen_event_bt_model(provider, c, :start),
    gen_event_bt_model(provider, c, :stop)]
  }
}.flatten(2)

ze_events = YAML::load_file(File.join(SRC_DIR,"ze_events.yaml"))
event_classes += ze_events.collect { |provider, es|
  es["events"].collect { |event|
    gen_extra_event_bt_model(provider, event)
  }
}.flatten

def get_structs_types(namespace, types, structs)
  types.select { |t|
    t.type.kind_of?(YAMLCAst::Struct) && (struct = structs.find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
  }.map(&:name).reject { |n|
    n.start_with?("#{namespace}_base_")
  }.to_set
end

def gen_struct_event_bt_model(provider, struct)
  {
    name: "#{provider}:#{struct}",
    payload: [
      {
        name: "p",
        cast_type: "#{struct} *",
        class: "unsigned",
        class_properties: {
          field_value_range: 64,
          preferred_display_base: 16 }
      },
      {
        name: "_p_val_length",
        cast_type: "size_t",
        class: "unsigned",
        class_properties: {
          field_value_range: 64 }
      },
      {
        name: "p_val",
        cast_type: "#{struct} *",
        class: "string",
        be_class: "ZE::#{to_class_name(struct)}"
      }
    ]
  }
end

event_classes +=
[[:lttng_ust_ze_structs, get_structs_types(:ze, $ze_api["typedefs"], $ze_api["structs"])],
 [:lttng_ust_zet_structs, get_structs_types(:zet, $zet_api["typedefs"], $zet_api["structs"])],
 [:lttng_ust_zes_structs, get_structs_types(:zes, $zes_api["typedefs"], $zes_api["structs"])],
 [:lttng_ust_zel_structs, get_structs_types(:zel, $zel_api["typedefs"], $zel_api["structs"])]].collect { |provider, structs|
  structs.collect { |struct|
    gen_struct_event_bt_model(provider, struct)
  }
}.flatten


environment = [
  {
    name: 'hostname',
    class: 'string',
  }
]

packet_context = [
  {
    name: 'cpu_id',
    class: 'unsigned',
    cast_type: 'uint64_t',
    class_properties: {
      field_value_range: 32
    }
  }
]

common_context = [
  {
    name: 'vpid',
    class: 'signed',
    cast_type: 'int64_t',
    class_properties: {
      field_value_range: 64,
    }
  },
  {
    name: 'vtid',
    class: 'unsigned',
    cast_type: 'uint64_t',
    class_properties: {
      field_value_range: 64,
    }
  }
]

puts YAML.dump({
  name: "thapi_ze",
  environment: environment,
  clock_snapshot_value: true,
  packet_context: packet_context,
  common_context: common_context,
  event_classes: event_classes })
