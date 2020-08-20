require_relative 'gen_ze_library_base.rb'

meta_parameter_lambda = lambda { |m, dir|
  if dir == :start
    lttng = m.lttng_in_type
  else
    lttng = m.lttng_out_type
  end
  name = lttng.name
  t = m.command[m.name].type.type
  case m
  when ScalarMetaParameter
    case t
    when YAMLCAst::Pointer
      "s << \"#{name}: \#{\"0x%016x\" % defi[\"#{name}\"]}\""
    when YAMLCAst::CustomType
      tname = t.name.sub(/_flags_t\Z/, "_flag_t")
      if $objects.include?(tname)
        "s << \"#{name}: \#{\"0x%016x\" % defi[\"#{name}\"]}\""
      elsif $all_enum_names.include?(tname)
        "s << \"#{name}: \#{ZE::#{to_class_name(tname)}.from_native(defi[\"#{name}\"], nil)}\""
      elsif $all_bitfield_names.include?(tname)
        "s << \"#{name}: [ \#{ZE::#{to_class_name(tname)}.from_native(defi[\"#{name}\"], nil).join(\", \")} ]\""
      elsif $all_struct_names.include?(tname)
        "s << \"#{name}: \#{defi[\"#{name}\"].size > 0 ? ZE::#{to_class_name(tname)}.new(FFI::MemoryPointer.from_string(defi[\"#{name}\"])) : nil}\""
      else
        "s << \"#{name}: \#{defi[\"#{name}\"]}\""
      end
    else
      "s << \"#{name}: \#{defi[\"#{name}\"]}\""
    end
  when ArrayMetaParameter
    case t
    when YAMLCAst::Pointer
      "s << \"#{name}: [ \#{defi[\"#{name}\"].collect { |v| \"0x%016x\" % v }.join(\", \")} ]\""
    when YAMLCAst::CustomType
      if $objects.include?(t.name)
        "s << \"#{name}: [ \#{defi[\"#{name}\"].collect { |v| \"0x%016x\" % v }.join(\", \")} ]\""
      elsif $all_struct_names.include?(t.name)
        "s << \"#{name}: [ \#{p = FFI::MemoryPointer.from_string(defi[\"#{name}\"]); sz = ZE::#{to_class_name(t.name)}.size; n = p.size / sz; (0...n).collect { |i| ZE::#{to_class_name(t.name)}.new(p + i*sz).to_s}.join(\", \")} ]\""
      else
        "s << \"#{name}: \#{defi[\"#{name}\"]}\""
      end
    else
      "s << \"#{name}: \#{defi[\"#{name}\"]}\""
    end
  else
    "s << \"#{name}: \#{defi[\"#{name}\"]}\""
  end
}

gen_event_lambda = lambda { |provider, c, dir|
  puts <<EOF
$event_lambdas["#{provider}:#{c.name}_#{dir}"] = lambda { |defi|
  s = "{ "
EOF
  fields = []
  if dir == :start
    if c.parameters
      fields += (c.parameters+c.tracepoint_parameters).collect { |p|
        case p.type
        when YAMLCAst::Pointer
          "s << \"#{p.name}: \#{\"0x%016x\" % defi[\"#{p.name}\"]}\""
        when YAMLCAst::CustomType
          t = p.type.name.sub(/_flags_t\Z/, "_flag_t")
          if $objects.include?(t)
            "s << \"#{p.name}: \#{\"0x%016x\" % defi[\"#{p.name}\"]}\""
          elsif $all_enum_names.include?(t)
            "s << \"#{p.name}: \#{ZE::#{to_class_name(t)}.from_native(defi[\"#{p.name}\"], nil)}\""
          elsif $all_bitfield_names.include?(t)
            "s << \"#{p.name}: [ \#{ZE::#{to_class_name(t)}.from_native(defi[\"#{p.name}\"], nil).join(\", \")} ]\""
          else
            "s << \"#{p.name}: \#{defi[\"#{p.name}\"]}\""
          end
        else
          "s << \"#{p.name}: \#{defi[\"#{p.name}\"]}\""
        end
      }
      fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
        meta_parameter_lambda.call(m, :start)
      }
    end
  else
    fields.push "s << \"#{RESULT_NAME}: \#{ZE::ZEResult.from_native(defi[\"#{RESULT_NAME}\"], nil)}\""
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
        meta_parameter_lambda.call(m, :stop)
      }
  end
  puts <<EOF
  #{fields.join("\n  s << ', '\n  ")}
EOF
  puts <<EOF
  s << " }"
}
EOF
}

puts <<EOF
require_relative 'ze_library.rb'

$event_lambdas = {}
EOF

provider = :lttng_ust_ze
$ze_commands.each { |c|
  gen_event_lambda.call(provider, c, :start)
  gen_event_lambda.call(provider, c, :stop)
}

provider = :lttng_ust_zet
$zet_commands.each { |c|
  gen_event_lambda.call(provider, c, :start)
  gen_event_lambda.call(provider, c, :stop)
}

provider = :lttng_ust_zes
$zes_commands.each { |c|
  gen_event_lambda.call(provider, c, :start)
  gen_event_lambda.call(provider, c, :stop)
}

extra_events = YAML::load_file("ze_events.yaml")

extra_events.each { |provider, h|
  h["events"].each { |e|
    puts <<EOF
$event_lambdas["#{provider}:#{e["name"]}"] = lambda { |defi|
  s = "{ "
EOF
    fields = e["fields"].collect { |f|
      field = LTTng::TracepointField::new(*f)
      case field.macro
      when :ctf_integer_hex
        "s << \"#{field.name}: \#{\"0x%016x\" % defi[\"#{field.name}\"]}\""
      when :ctf_integer
        arg = e["args"].find { |type, name|
          name == field.expression
        }
        if arg
          t = arg[0]
          t = t.sub(/_flags_t\Z/, "_flag_t") if t
          if $all_enum_names.include?(t)
            "s << \"#{field.name}: \#{ZE::#{to_class_name(t)}.from_native(defi[\"#{field.name}\"], nil)}\""
          elsif $all_bitfield_names.include?(t)
            "s << \"#{field.name}: [ \#{ZE::#{to_class_name(t)}.from_native(defi[\"#{field.name}\"], nil)} ]\""
          else
            "s << \"#{field.name}: \#{defi[\"#{field.name}\"]}\""
          end
        else
          "s << \"#{field.name}: \#{defi[\"#{field.name}\"]}\""
        end
      else
        raise "Unsupported LTTng macro #{field.macro}!"
      end
    }
    puts <<EOF
  #{fields.join("\n  s << ', '\n  ")}
EOF
    puts <<EOF
  s << " }"
}
EOF
  }
}
