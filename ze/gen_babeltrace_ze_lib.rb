require_relative 'gen_ze_library_base.rb'

puts <<EOF
require_relative 'ze_library.rb'
EOF

YAML.load_file("ze_babeltrace_model.yaml")[:event_classes].each { |e|
  puts <<EOF
$event_lambdas["#{e[:name]}"] = lambda { |defi|
  s = "{ "
EOF
  fields = e[:payload].reject { |f|
    f[:name].start_with?("_") && f[:name].end_with?("_length")
  }.collect { |f|
    name = f[:name]
    case f[:class]
    when "signed", "unsigned"
      tname = f[:cast_type].sub(/_flags_t\Z/, "_flag_t")
      if $all_enum_names.include?(tname)
        "s << \"#{name}: \#{ZE::#{to_class_name(tname)}.from_native(defi[\"#{name}\"], nil)}\""
      elsif $all_bitfield_names.include?(tname)
        "s << \"#{name}: [ \#{ZE::#{to_class_name(tname)}.from_native(defi[\"#{name}\"], nil).join(\", \")} ]\""
      elsif f[:class_properties][:preferred_display_base] == 16
        "s << \"#{name}: \#{\"0x%016x\" % defi[\"#{name}\"]}\""
      else
        "s << \"#{name}: \#{defi[\"#{name}\"]}\""
      end
    when "double"
      "s << \"#{name}: \#{defi[\"#{name}\"]}\""
    when "string"
      if f[:be_class]
        if name.end_with?("_vals")
          "s << \"#{name}: [ \#{p = FFI::MemoryPointer.from_string(defi[\"#{name}\"]); sz = #{f[:be_class]}.size; n = p.size / sz; (0...n).collect { |i| #{f[:be_class]}.new(p + i*sz).to_s}.join(\", \")} ]\""
        else
          "s << \"#{name}: \#{defi[\"#{name}\"].size > 0 ? #{f[:be_class]}.new(FFI::MemoryPointer.from_string(defi[\"#{name}\"])) : nil}\""
        end
      else
        "s << \"#{name}: \#{defi[\"#{name}\"].inspect}\""
      end
    when "array_dynamic"
      case f[:field][:class]
      when "unsigned"
        if f[:field][:class_properties][:preferred_display_base] == 16
          "s << \"#{name}: [ \#{defi[\"#{name}\"].collect { |v| \"0x%016x\" % v }.join(\", \")} ]\""
        else
          "s << \"#{name}: \#{defi[\"#{name}\"]}\""
        end
      else
        "Unsupported field type: #{f}"
      end
    else
      raise "Unsupported field type: #{f}"
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
