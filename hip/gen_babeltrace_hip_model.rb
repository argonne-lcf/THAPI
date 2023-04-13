require_relative 'gen_hip_library_base.rb'
require_relative '../utils/gen_babeltrace_model_helper'

$integer_sizes = {
  "unsigned char" => 8,
  "char" => 8,
  "uint8_t" => 8,
  "int8_t" =>  8,
  "unsigned short" => 16,
  "unsigned short int" => 16,
  "short" => 16,
  "short int" => 16,
  "unsigned int" => 32,
  "int" => 32,
  "uint32_t" => 32,
  "int32_t" => 32,
  "unsigned long long" => 64,
  "unsigned long long int" => 64,
  "long long" => 64,
  "long long int" => 64,
  "long" => 64,
  "long int" => 64,
  "unsigned long" => 64,
  "unsigned long int" => 64,
  "uint64_t" => 64,
  "int64_t" => 64,
  "uintptr_t" => 64,
  "intptr_t" => 64,
  "size_t" => 64,
  "ssize_t" => 64,
}

$all_enums.each { |t|
  $integer_sizes["enum #{t.name}"] = 32
}

HIP_ENUM_SCALARS.each { |t|
  $integer_sizes[t] = 32
}

find_types_map($all_types, YAMLCAst::Int, $integer_sizes)

$integer_signed = {
  "char" => true,
  "int8_t" =>  true,
  "short" => true,
  "short int" => true,
  "int" => true,
  "int32_t" => true,
  "long long" => true,
  "long long int" => true,
  "long" => true,
  "long int" => true,
  "int64_t" => true,
  "intptr_t" => true,
  "ssize_t" => true,
  "unsigned char" => false,
  "uint8_t" => false,
  "unsigned short" => false,
  "unsigned short int" => false,
  "unsigned int" => false,
  "uint32_t" => false,
  "unsigned long long" => false,
  "unsigned long long int" => false,
  "unsigned long" => false,
  "unsigned long int" => false,
  "uint64_t" => false,
  "uintptr_t" => false,
  "size_t" => false,
}

$all_enums.each { |t|
  $integer_signed["enum #{t.name}"] = true
}

HIP_ENUM_SCALARS.each { |t|
  $integer_signed[t] = true
}

find_types_map($all_types, YAMLCAst::Int, $integer_signed)

def integer_size(t)
  return 64 if t.match(/\*/)
  return 64 if $objects.include?(t)
  return 64 if HIP_POINTER_TYPES.include?(t)
  r = $integer_sizes[t]
  raise "unknown integer type #{t}" if r.nil?
  r
end

def integer_signed?(t)
  return false if t.match(/\*/)
  return false if $objects.include?(t)
  return false if HIP_POINTER_TYPES.include?(t)
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
  when ArrayMetaParameter, InString, OutString, OutLTTng, InLTTng, ReturnString
    if lttng.macro.to_s == "ctf_string"
      [["ctf_string", "#{t} *", "#{name}", lttng]]
    else
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  when ArrayByRefMetaParameter
    [["ctf_integer", "size_t", "_#{name}_length", nil],
     [lttng.macro.to_s, "#{t.type} *", "#{name}", lttng]]
  when FixedArrayMetaParameter
    [[lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
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
    r = c.type.lttng_type
    fields = if r
        [[r.macro.to_s, c.type.to_s, "#{RESULT_NAME}", r]]
      else
        []
    end
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_types_name(m, :stop)
    }.flatten(1)
  end
  fields
end

event_classes = 
[[:lttng_ust_hip, $hip_commands]
].collect { |provider, commands|
  commands.collect { |c|
    [gen_event_bt_model(provider, c, :start),
    gen_event_bt_model(provider, c, :stop)]
  }
}.flatten(2)

hip_events = YAML::load_file(File.join(SRC_DIR,"hip_events.yaml"))
event_classes += hip_events.collect { |provider, es|
  es["events"].collect { |event|
    gen_extra_event_bt_model(provider, event)
  }
}.flatten

puts YAML.dump({ name: "thapi_hip", event_classes: event_classes })

