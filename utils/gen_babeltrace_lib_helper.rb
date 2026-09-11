require_relative 'yaml_ast'
require_relative 'gen_probe_base'

# The whole of a backend's babeltrace-library generator: require the FFI
# bindings, then emit one pretty-printer per event in the model.
#
# `meta_parameters_function` is what the backend declares about the byte-array
# parameters of its functions; a backend that declares none passes nothing.
def print_babeltrace_lib(naming, meta_parameters_function = {})
  puts "require_relative '#{naming.backend}_library.rb'"
  add_babeltrace_event_callbacks(naming, "btx_#{naming.backend}_model.yaml", meta_parameters_function)
end

# One `$event_lambdas` entry per event: a lambda that renders the event's
# payload as a string.
def add_babeltrace_event_callbacks(naming, file, meta_parameters_function)
  event_classes = yaml_load_file_cached(file)[:stream_classes].flat_map { |s| s[:event_classes] }
  check_meta_parameters_function(meta_parameters_function, byte_array_parameters(event_classes))

  event_classes.each do |e|
    # Handle payload_field_class not present, in this case empty array
    members = e[:payload_field_class]&.[](:members).to_a
    renderers = meta_parameters_function.fetch(event_function_name(e[:name]), {})
    fields = members.reject { |f| length_field_name?(f[:name]) }
                    .map { |f| render_field(naming, f, renderers[parameter_name(f[:name])]) }

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

# An event's name carries the provider that declares it and the direction it
# reports, around the name of the function it belongs to.
#
#   >> event_function_name('lttng_ust_cuda:cuDeviceGetLuid_exit')
#   => "cuDeviceGetLuid"
def event_function_name(event_name)
  event_name.split(':').last.sub(/_(#{START}|#{STOP})\z/, '')
end

# The parameter a payload field carries. A tracepoint decorates the name it
# traces a parameter under -- cuDeviceGetLuid's `luid` is traced as `luid_vals`
# -- and a row names the parameter, which is what the header calls it.
#
#   >> parameter_name('luid_vals')
#   => "luid"
def parameter_name(field_name)
  field_name.sub(/_vals?\z/, '')
end

# The byte-array parameters the model carries, as `{ function => [parameter] }`
# -- the only ones a renderer can read. A byte array reaches the payload as a
# string, whichever of char, unsigned char or uint8_t the header spells it with.
#
# One function's parameters are gathered from all of its events, because a
# direction says only which event carries a parameter, never how it prints.
def byte_array_parameters(event_classes)
  event_classes.group_by { |e| event_function_name(e[:name]) }.transform_values do |events|
    events.flat_map { |e| e[:payload_field_class]&.[](:members).to_a }
          .select { |f| f[:field_class][:type] == 'string' }
          .collect { |f| parameter_name(f[:name]) }
  end
end

# Raise unless every row names a traced function and a byte-array parameter of
# it -- checked once, before a single line is generated. This is
# `check_meta_parameters_struct` for a function's parameters, and fails the
# same way: a name that is not there and one that is not bytes both leave the
# renderer nothing to read, which would otherwise raise at read time.
def check_meta_parameters_function(meta_parameters_function, byte_arrays)
  meta_parameters_function.each do |function, parameters|
    bytes = byte_arrays.fetch(function) do
      raise "meta_parameters_function names no traced function: #{function}"
    end
    unrenderable = parameters.keys - bytes
    next if unrenderable.empty?

    raise "#{function} traces no byte-array parameter #{unrenderable.join(', ')} " \
          "(traces #{bytes.empty? ? 'none' : bytes.join(', ')})"
  end
end

# The statement that appends one field to the rendered payload. `be_class` is
# the FFI class for a field whose raw bytes mean something richer -- an enum, a
# bitmask, a struct -- and is absent for one that prints as itself. `renderer`
# is the `Bytes` function the backend declared for this field, for bytes that
# mean something no type says.
def render_field(naming, field, renderer = nil)
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
    if renderer
      %{s << "#{name}: \#{#{naming.module_name}::Bytes.#{renderer}(defi["#{name}"].bytes)}"}
    elsif be_class
      render_packed_struct(name, be_class)
    else
      %(s << "#{name}: \#{defi["#{name}"].inspect}")
    end
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
  if naming.api.bitfield?(field_class[:cast_type])
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
