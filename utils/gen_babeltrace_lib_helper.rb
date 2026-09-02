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
      members = e[:payload_field_class]&.[](:members).to_a
      fields = members.reject { |f| /^_.*_length$/ =~ f[:name] }
                      .map { |f| render_field(naming, f) }

      # Now just print the full strings to pretty printf the struct
      puts <<~EOF
        $event_lambdas["#{e[:name]}"] = lambda { |defi|
          s = "{ "
          #{fields.join("\n  s << ', '\n  ")}
          s << " }"
        }
      EOF
    end
  end
end

# The statement that appends one field to the rendered payload. `be_class` is
# the FFI class for a field whose raw bytes mean something richer -- an enum, a
# bitmask, a struct -- and is absent for one that prints as itself.
def render_field(naming, field)
  name = field[:name]
  fc = field[:field_class]
  be_class = field[:metadata]&.[](:be_class)
  plain = %(s << "#{name}: \#{defi["#{name}"]}")

  case fc[:type]
  when 'integer_signed', 'integer_unsigned'
    if be_class
      render_named_integer(naming, name, fc, be_class)
    elsif fc[:preferred_display_base] == 16
      %(s << "#{name}: \#{"0x%016x" % defi["#{name}"]}")
    else
      plain
    end
  when 'double', 'single'
    plain
  when 'string'
    be_class ? render_packed_struct(name, be_class) : %(s << "#{name}: \#{defi["#{name}"].inspect}")
  when 'array_dynamic', 'array_static'
    element = fc[:element_field_class]
    unless %w[integer_signed integer_unsigned].include?(element[:type])
      raise "Unsupported field type for array: #{element[:type]}"
    end

    if element[:preferred_display_base] == 16
      %{s << "#{name}: [ \#{defi["#{name}"].collect { |v| "0x%016x" % v }.join(", ")} ]"}
    else
      plain
    end
  else
    raise "Unsupported field type: #{fc[:type]}"
  end
end

# A bitmask's value is a set of flags, so it renders as a list; a plain enum
# renders as the single name it stands for.
def render_named_integer(naming, name, field_class, be_class)
  if naming.api.bitfield_names.include?(field_class[:cast_type])
    %{s << "#{name}: [ \#{#{be_class}.from_native(defi["#{name}"], nil).join(", ")} ]"}
  else
    %{s << "#{name}: \#{#{be_class}.from_native(defi["#{name}"], nil)}"}
  end
end

# A struct traced as its raw bytes. A `_vals` field holds an array of them, so
# it is split on the struct's own size; anything else holds one, and an empty
# string means the traced pointer was null.
def render_packed_struct(name, be_class)
  if name.end_with?('_vals')
    %{s << "#{name}: [ \#{p = FFI::MemoryPointer.from_string(defi["#{name}"]); sz = #{be_class}.size; n = p.size / sz; (0...n).collect { |i| #{be_class}.new(p + i*sz).to_s}.join(", ")} ]"}
  else
    %{s << "#{name}: \#{defi["#{name}"].size > 0 ? #{be_class}.new(FFI::MemoryPointer.from_string(defi["#{name}"])) : nil}"}
  end
end
