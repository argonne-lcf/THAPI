# The whole of a backend's babeltrace-library generator: require the FFI
# bindings, then emit one pretty-printer per event in the model.
def print_babeltrace_lib(naming, backend)
  puts "require_relative '#{backend}_library.rb'"
  add_babeltrace_event_callbacks(naming, "btx_#{backend}_model.yaml")
end

# One `$event_lambdas` entry per event: a lambda that renders the event's
# payload as a string.
def add_babeltrace_event_callbacks(naming, file)
  YAML.load_file(file)[:stream_classes].each do |s|
    s[:event_classes].each do |e|
      # Handle payload_field_class not present, in this case empty array
      m = e[:payload_field_class]&.[](:members).to_a

      fields = m.reject { |f| /^_.*_length$/ =~ f[:name] }
                .map do |f|
        name = f[:name]
        fc = f[:field_class]
        be_class = f[:metadata]&.[](:be_class)

        default_command = %(s << "#{name}: \#{defi["#{name}"]}")

        case fc[:type]
        when 'integer_signed', 'integer_unsigned'
          if be_class
            if naming.api.bitfield_names.include?(fc[:cast_type])
              %{s << "#{name}: [ \#{#{be_class}.from_native(defi["#{name}"], nil).join(", ")} ]"}
            else
              %{s << "#{name}: \#{#{be_class}.from_native(defi["#{name}"], nil)}"}
            end
          elsif fc[:preferred_display_base] == 16
            %(s << "#{name}: \#{"0x%016x" % defi["#{name}"]}")
          else
            default_command
          end
        when 'double', 'single'
          default_command
        when 'string'
          if be_class
            if name.end_with?('_vals')
              %{s << "#{name}: [ \#{p = FFI::MemoryPointer.from_string(defi["#{name}"]); sz = #{be_class}.size; n = p.size / sz; (0...n).collect { |i| #{be_class}.new(p + i*sz).to_s}.join(", ")} ]"}
            else
              %{s << "#{name}: \#{defi["#{name}"].size > 0 ? #{be_class}.new(FFI::MemoryPointer.from_string(defi["#{name}"])) : nil}"}
            end
          else
            %(s << "#{name}: \#{defi["#{name}"].inspect}")
          end
        when 'array_dynamic', 'array_static'
          case fc[:element_field_class][:type]
          when 'integer_signed', 'integer_unsigned'
            if fc[:element_field_class][:preferred_display_base] == 16
              %{s << "#{name}: [ \#{defi["#{name}"].collect { |v| "0x%016x" % v }.join(", ")} ]"}
            else
              default_command
            end
          else
            raise "Unsupported field type for array: #{fc[:element_field_class][:type]}"
          end
        else
          raise "Unsupported field type: #{fc[:type]}"
        end
      end.join("\n  s << ', '\n  ")

      # Now just print the full strings to pretty printf the struct
      puts <<~EOF
        $event_lambdas["#{e[:name]}"] = lambda { |defi|
          s = "{ "
          #{fields}
          s << " }"
        }
      EOF
    end
  end
end
