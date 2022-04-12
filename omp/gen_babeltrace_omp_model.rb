require_relative 'gen_omp_library_base.rb'
require_relative '../utils/gen_babeltrace_model_helper'

$integer_sizes = {
  "unsigned char" => 8,
  "char" => 8,
  "uint8_t" => 8,
  "int8_t" =>  8,
  "unsigned short" => 16,
  "unsigned short int" => 16,
  "short" => 16,
  "uint16_t" => 16,
  "int16_t" => 16,
  "unsigned int" => 32,
  "int" => 32,
  "uint32_t" => 32,
  "int32_t" => 32,
  "unsigned long long" => 64,
  "unsigned long long int" => 64,
  "long" => 64,
  "long int" => 64,
  "unsigned long" => 64,
  "unsigned long int" => 64,
  "long long" => 64,
  "long long int" => 64,
  "uint64_t" => 64,
  "int64_t" => 64,
  "uintptr_t" => 64,
  "size_t" => 64,
  "ompt_id_t" => 64,
  "ompt_device_time_t"=> 64,
  "ompt_buffer_cursor_t" => 64,
  "ompt_hwid_t" => 64,
  "ompt_wait_id_t"=> 64,
  "ompd_size_t" => 64,
  "ompd_wait_id_t" => 64,
  "ompd_addr_t" => 64,
  "ompd_word_t" => 64,
  "ompd_seg_t" => 64,
  "ompd_device_t" => 64,
  "ompd_thread_id_t" => 64,
  "ompd_icv_id_t" => 64,
  "ompt_task_flags_t" => 32
}

OMPT_ENUM_SCALARS.each { |t|
  $integer_sizes[t] = 32
}

$int_scalars.each { |t, v|
  $integer_sizes[t] = $integer_sizes[v]
}

$integer_signed = {
  "char" => true,
  "int8_t" => true,
  "short" => true,
  "int16_t" => true,
  "int" => true,
  "int32_t" => true,
  "long" => true,
  "long int" => true,
  "long long" => true,
  "long long int" => true,
  "int64_t" => true,
  "unsigned char" => false,
  "uint8_t" => false,
  "unsigned short" => false,
  "unsigned short int" => false,
  "uint16_t" => false,
  "unsigned int" => false,
  "uint32_t" => false,
  "unsigned long" => false,
  "unsigned long int" => false,
  "unsigned long long" => false,
  "unsigned long long int" => false,
  "uint64_t" => false,
  "uintptr_t" => false,
  "size_t" => false,
  "ompt_id_t" =>false,
  "ompt_device_time_t"=>false,
  "ompt_buffer_cursor_t" =>false,
  "ompt_hwid_t" =>false,
  "ompt_wait_id_t"=>false,
  "ompd_size_t" =>false,
  "ompd_wait_id_t" =>false,
  "ompd_addr_t" =>false,
  "ompd_word_t" =>true,
  "ompd_seg_t" =>false,
  "ompd_device_t" =>false,
  "ompd_thread_id_t" =>false,
  "ompd_icv_id_t" => false,
  "ompt_task_flags_t" => true
}

$function_pointer = ["ompt_function_lookup_t"]

OMPT_ENUM_SCALARS.each { |t|
  $integer_signed[t] = true
}

$int_scalars.each { |t, v|
  $integer_signed[t] = $integer_signed[v]
}

def integer_size(t)
  return 64 if t.match(/\*/)
  return 64 if $objects.include?(t)
  return 64 if $function_pointer.include?(t)
  r = $integer_sizes[t]
  raise "unknown integer type #{t}" if r.nil?
  r
end

def integer_signed?(t)
  return false if t.match(/\*/)
  return false if $objects.include?(t)
  return false if $function_pointer.include?(t)
  r = $integer_signed[t]
  raise "unknown integer type #{t}" if r.nil?
  r
end

def meta_parameter_types_name(m)
  lttng = m.lttng_in_type
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
  when ArrayMetaParameter, InString, OutString, OutLTTng, InLTTng
    if lttng.macro.to_s == "ctf_string"
      [["ctf_string", "#{t} *", "#{name}", lttng]]
    else
      [["ctf_integer", "size_t", "_#{name}_length", nil],
       [lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
    end
  when FixedArrayMetaParameter
    [[lttng.macro.to_s, "#{t} *", "#{name}", lttng]]
  when OutPtrString
    [["ctf_string", "#{t}", "#{name}", lttng]]
  else
    raise "unsupported meta parameter class #{m.class} #{lttng.call_string} #{t}"
  end
end

def get_fields_types_name(c)
  fields = (c.parameters ? c.parameters : []).collect { |p|
    [p.lttng_type.macro.to_s, p.type.to_s, p.name.to_s, p.lttng_type]
  }
  fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
    meta_parameter_types_name(m, :start)
  }.flatten(1)
  fields
end

def gen_event_fields_bt_model(c)
  types_name = get_fields_types_name(c)
  types_name.collect { |lttng_name, type, name, lttng|
    gen_bt_field_model(lttng_name, type.sub(/\Aconst /, ""), name, lttng)
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

