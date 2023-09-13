require_relative 'gen_omp_library_base.rb'
require_relative '../utils/gen_babeltrace_model_helper'

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

def meta_parameter_types_name(m)
  lttng = m.lttng_type
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
  when ArrayMetaParameter, StringMetaParameter
    if lttng.macro.to_s == "ctf_string"
      [["ctf_string", "#{t} *", "#{name}", lttng]]
    else
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  else
    raise "unsupported meta parameter class #{m.class} #{lttng.call_string} #{t}"
  end
end

def get_fields_types_name(c)
  fields = (c.parameters ? c.parameters : []).collect { |p|
    [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
  }
  fields += c.meta_parameters.collect { |m|
    meta_parameter_types_name(m)
  }.flatten(1)
  fields
end

def gen_event_fields_bt_model(c)
  types_name = get_fields_types_name(c)
  types_name.collect { |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ""), name, lttng, be_union: true)
  }
end


def gen_event_bt_model(provider, c)
  { name: "#{provider}:#{c.name.gsub(/_func\z/,'')}",
    payload: gen_event_fields_bt_model(c) }
end

event_classes = 
[[:lttng_ust_ompt, $ompt_commands],
].collect { |provider, commands|
  commands.collect { |c|
    [gen_event_bt_model(provider, c)]
  }
}.flatten(2)

puts YAML.dump({ name: "thapi_omp", event_classes: event_classes })

