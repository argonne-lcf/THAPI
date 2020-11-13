require_relative 'gen_ze_library_base.rb'
puts <<EOF
#include <ze_api.h>
#include <ze_ddi.h>
#include <zet_api.h>
#include <zet_ddi.h>
#include <zes_api.h>
#include <zes_ddi.h>
#include <layers/zel_tracing_api.h>
#include <layers/zel_tracing_ddi.h>
#include <babeltrace2/babeltrace.h>
#include "babeltrace_ze.h"
#include "babeltrace_ze_callbacks.h"

EOF

def print_field_member_access(lttng, type, name, i, array_type: nil)
  puts <<EOF
  {
    const bt_field *_field = NULL;
    _field = bt_field_structure_borrow_member_field_by_index_const(payload_field, #{i});
EOF
  case lttng
  when "ctf_float"
    puts <<EOF
    bt_field_class_type _type = bt_field_get_class_type(_field);
    if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_SINGLE_PRECISION_REAL))
      #{name} = (#{type})bt_field_real_single_precision_get_value(_field);
    else
      #{name} = (#{type})bt_field_real_double_precision_get_value(_field);
EOF
  when "ctf_integer", "ctf_integer_hex", "ctf_enum"
    puts <<EOF
    bt_field_class_type _type = bt_field_get_class_type(_field);
    if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER))
      #{name} = (#{type})bt_field_integer_unsigned_get_value(_field);
    else
      #{name} = (#{type})bt_field_integer_signed_get_value(_field);
EOF
  when "ctf_sequence", "ctf_sequence_hex", "ctf_array"
    scalar_type = array_type
    puts <<EOF
    uint64_t _sz = bt_field_array_get_length(_field);
    if (_sz > 0) {
      bt_field_class_type _type = bt_field_get_class_type(bt_field_array_borrow_element_field_by_index_const(_field, 0));
      #{name} = (#{type})malloc(_sz*sizeof(#{scalar_type}));
      for (uint64_t _i = 0; _i < _sz; _i++) {
        if (bt_field_class_type_is(_type, BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER))
          ((#{scalar_type} *)#{name})[_i] = (#{scalar_type})bt_field_integer_unsigned_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
        else
          ((#{scalar_type} *)#{name})[_i] = (#{scalar_type})bt_field_integer_signed_get_value(bt_field_array_borrow_element_field_by_index_const(_field, _i));
      }
    } else
      #{name} = NULL;
EOF
  when "ctf_sequence_text", "ctf_string", "ctf_array_text"
    if !type.to_s.match(/\*/)
      puts <<EOF
    if (bt_field_string_get_value(_field))
      memcpy(&#{name}, bt_field_string_get_value(_field), sizeof(#{name}));
EOF
    else
      puts <<EOF
    #{name} = (#{type})bt_field_string_get_value(_field);
EOF
    end
  else
    raise "unsupported data type #{lttng.inspect}"
  end
  puts <<EOF
  }
EOF
end

def print_field_member_free(lttng, name)
  case lttng
  when "ctf_sequence", "ctf_sequence_hex", "ctf_array"
    puts <<EOF
  free((void*)#{name});
EOF
  end
end

def get_lttng_type(m, dir)
  if dir == :start
    lttng = m.lttng_in_type
  else
    lttng = m.lttng_out_type
  end
end

def print_meta_parameter_field_members_free(m, dir)
  lttng = get_lttng_type(m, dir)
  name = lttng.name
  print_field_member_free(lttng.macro.to_s, name.to_s)
end

def print_field_members_free(c, dir)
  if dir == :start
    ((c.parameters ? c.parameters : []) + c.tracepoint_parameters).each { |p|
      lttng = p.lttng_type
      print_field_member_free(lttng.macro.to_s, p.name)
    }
    c.meta_parameters.select { |m| m.kind_of?(In) }.each { |m|
       print_meta_parameter_field_members_free(m, dir)
    }
  else
    c.meta_parameters.select { |m| m.kind_of?(Out) }.each { |m|
       print_meta_parameter_field_members_free(m, dir)
    }
  end
end

def print_meta_parameter_field_members_access(m, dir, i)
  lttng = get_lttng_type(m, dir)
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      print_field_member_access("ctf_integer", "size_t", "_#{name}_length", i)
      i += 1
      print_field_member_access(lttng.macro.to_s, "#{t} *", "#{name}", i, array_type: lttng.type.to_s)
      i += 1
    else
      print_field_member_access(lttng.macro.to_s, "#{t}", "#{name}", i)
      i += 1
    end
  when ArrayMetaParameter, InString, OutString
    unless lttng.macro.to_s == "ctf_string"
      print_field_member_access("ctf_integer", "size_t", "_#{name}_length", i)
      i += 1
    end
    print_field_member_access(lttng.macro.to_s, "#{t} *", "#{name}", i, array_type: lttng.type.to_s)
    i += 1
  end
  return i
end

def print_field_members_access(c, dir)
  if dir == :start
    i = 0
    ((c.parameters ? c.parameters : []) + c.tracepoint_parameters).each { |p|
      lttng = p.lttng_type
      print_field_member_access(lttng.macro.to_s, p.type, p.name, i)
      i += 1;
    }
    c.meta_parameters.select { |m| m.kind_of?(In) }.each { |m|
       i = print_meta_parameter_field_members_access(m, dir, i)
    }
  else
    i = 0
    print_field_member_access("ctf_integer", "ze_result_t", "#{RESULT_NAME}", i)
    i += 1
    c.meta_parameters.select { |m| m.kind_of?(Out) }.each { |m|
       i = print_meta_parameter_field_members_access(m, dir, i)
    }
  end
end

def meta_parameter_name(m, dir)
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
      ["_#{name}_length", "#{name}"]
    else
      "#{name}"
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == "ctf_string"
      "#{name}"
    else
      ["_#{name}_length", "#{name}"]
    end
  end
end

def meta_parameter_decl(m, dir)
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
      ["size_t _#{name}_length;", "#{t} *#{name};"]
    else
      "#{t} #{name};"
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == "ctf_string"
      "#{t} *#{name};"
    else
      ["size_t _#{name}_length;", "#{t} *#{name};"]
    end
  end
end

def get_fields_decl(c, dir)
  if dir == :start
    fields = ((c.parameters ? c.parameters : []) + c.tracepoint_parameters).collect { |p|
      p.to_s + ";"
    }
    fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
      meta_parameter_decl(m, :start)
    }
  else
    fields = ["ze_result_t #{RESULT_NAME};"]
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_decl(m, :stop)
    }
  end
  fields.flatten!
  fields
end

def get_fields_names(c, dir)
  if dir == :start
    fields = ((c.parameters ? c.parameters : []) + c.tracepoint_parameters).collect { |p|
      p.name.to_s
    }
    fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
      meta_parameter_name(m, :start)
    }
  else
    fields = ["#{RESULT_NAME}"]
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_name(m, :stop)
    }
  end
  fields.flatten!
  fields
end

gen_event_dispatcher = lambda { |provider, c, dir|
  puts <<EOF
static void
#{provider}_#{c.name}_#{SUFFIXES[dir]}_dispatcher(
    struct ze_dispatch      *ze_dispatch,
    struct ze_callbacks     *callbacks,
    const bt_event          *bt_evt,
    const bt_clock_snapshot *bt_clock) {
EOF
  fields = get_fields_decl(c, dir)

puts <<EOF unless fields.empty?
  const bt_field *payload_field = bt_event_borrow_payload_field_const(bt_evt);
  #{fields.join("\n  ")}
EOF
  print_field_members_access(c, dir)
  puts <<EOF
  void **_p = NULL;
  while( (_p = utarray_next(callbacks->callbacks, _p)) ) {
    ((#{provider}_#{c.name}_#{SUFFIXES[dir]}_cb *)*_p)(
      #{(["bt_evt", "bt_clock"] + get_fields_names(c, dir)).join(",\n      ")});
  }
EOF
  print_field_members_free(c, dir)
  puts <<EOF
}

EOF
}

gen_extra_event_dispatcher = lambda { |provider, event|
    args = event["args"].collect { |arg| arg.reverse }.to_h
    lttng_fields = event["fields"].collect { |field|
      LTTng::TracepointField::new(*field)
    }
    param_fields = []
    lttng_fields.each { |lttng|
      name = lttng.name
      type = lttng.type
      if name.match(/_val\z/)
        n = name.sub(/_val\z/,"")
        param_fields.push ["ctf_integer", "size_t", "_#{name}_length"]
        type = args[n] if args[n]
      else
        type = args[name] if args[name]
      end
      param_fields.push [lttng.macro.to_s, type.to_s, name.to_s]
    }
    fields_decl = []
    param_fields.each { |param_field|
      fields_decl.push "#{param_field[1]} #{param_field[2]}"
    }
    puts <<EOF
static void
#{provider}_#{event["name"]}_dispatcher(
    struct ze_dispatch      *ze_dispatch,
    struct ze_callbacks     *callbacks,
    const bt_event          *bt_evt,
    const bt_clock_snapshot *bt_clock) {
  const bt_field *payload_field = bt_event_borrow_payload_field_const(bt_evt);
  #{fields_decl.join(";\n  ")};
EOF
    param_fields.each_with_index { |param_field, i|
      print_field_member_access(param_field[0], param_field[1], param_field[2], i)
    }
    puts <<EOF
  void **_p = NULL;
  while( (_p = utarray_next(callbacks->callbacks, _p)) ) {
    ((#{provider}_#{event["name"]}_cb *)*_p)(
      #{(["bt_evt", "bt_clock"] + param_fields.collect { |param_field| param_field[2] }).join(",\n      ")});
  }
EOF
  puts <<EOF
}

EOF
}

provider = :lttng_ust_ze
$ze_commands.each { |c|
  gen_event_dispatcher.call(provider, c, :start)
  gen_event_dispatcher.call(provider, c, :stop)
}

provider = :lttng_ust_zet
$zet_commands.each { |c|
  gen_event_dispatcher.call(provider, c, :start)
  gen_event_dispatcher.call(provider, c, :stop)
}

provider = :lttng_ust_zes
$zes_commands.each { |c|
  gen_event_dispatcher.call(provider, c, :start)
  gen_event_dispatcher.call(provider, c, :stop)
}

provider = :lttng_ust_zel
$zel_commands.each { |c|
  gen_event_dispatcher.call(provider, c, :start)
  gen_event_dispatcher.call(provider, c, :stop)
}

ze_events = YAML::load_file(File.join(SRC_DIR,"ze_events.yaml"))

ze_events.each { |provider, es|
  es["events"].each { |event|
    gen_extra_event_dispatcher.call(provider, event)
  }
}

gen_event_dispatch_init = lambda { |provider, c, dir|
  puts <<EOF
  ze_register_dispatcher(ze_dispatch, "#{provider}:#{c.name}_#{SUFFIXES[dir]}", &#{provider}_#{c.name}_#{SUFFIXES[dir]}_dispatcher);
EOF
}

gen_extra_event_dispatch_init = lambda { |provider, e|
  puts <<EOF
  ze_register_dispatcher(ze_dispatch, "#{provider}:#{e["name"]}", &#{provider}_#{e["name"]}_dispatcher);
EOF
}

puts <<EOF
void init_dispatchers(struct ze_dispatch *ze_dispatch) {
EOF

provider = :lttng_ust_ze
$ze_commands.each { |c|
  gen_event_dispatch_init.call(provider, c, :start)
  gen_event_dispatch_init.call(provider, c, :stop)
}

provider = :lttng_ust_zet
$zet_commands.each { |c|
  gen_event_dispatch_init.call(provider, c, :start)
  gen_event_dispatch_init.call(provider, c, :stop)
}

provider = :lttng_ust_zes
$zes_commands.each { |c|
  gen_event_dispatch_init.call(provider, c, :start)
  gen_event_dispatch_init.call(provider, c, :stop)
}

provider = :lttng_ust_zel
$zel_commands.each { |c|
  gen_event_dispatch_init.call(provider, c, :start)
  gen_event_dispatch_init.call(provider, c, :stop)
}

ze_events.each { |provider, es|
  es["events"].each { |event|
    gen_extra_event_dispatch_init.call(provider, event)
  }
}

puts <<EOF
}
EOF


