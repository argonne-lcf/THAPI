require 'babeltrace2'
require 'yaml'
require 'optparse'

$options = {
  sink: 'text:details',
  schemas: [],
  trace: nil
}

OptionParser.new do |opts|
  opts.banner = "Usage: dust.rb [options] trace_file"
  opts.on("--sink PLUGIN", "Select sink plugin") do |sink|
    $options[:sink] = sink.split(":")
  end
  opts.on("-s", "--schemas x,y,z", Array, "List of schemas files to load") do |schemas|
    $options[:schemas] = schemas.collect { |path|
      schema = YAML.load_file(path)
      [schema[:name], schema]
    }.to_h
  end
  opts.on("-f", "--file FILENAME", "Configuration file Name") do |path|
    dust_schema = YAML.load_file(path)
    trace = dust_schema[:trace]
    plugins = dust_schema[:plugins]
    sink = plugins.select { |p| p.match(/^sink/) }.last
    if sink
      $options[:sink] = sink.split(":")[1..2]
    end
    $options[:trace] = trace
  end
end.parse!
$trace_file = ARGV[0] ? ARGV[0] : $options[:trace]
raise "trace file not specified" unless $trace_file
unless File.exist?($trace_file)
  $trace_file = File.join(ENV["DUST_TRACE_DIR"], $trace_file)
end

def create_datastructure(trace_class, l)
  # Utils functions
  def create_callback_aliases(aliases, root)
    aliases.each { |new|
      singleton_class.send(:alias_method, "callback_create_#{new}", "callback_create_#{root}")
    }
  end

  def populate_integer_field_class(field, d)
    field.preferred_display_base = d[:preferred_display_base] if d[:preferred_display_base]
    field.field_value_range = d[:field_value_range] if d[:field_value_range]
  end

  def populate_enumeration_class(field, d)
    populate_integer_field_class(field, d)
    d[:mapping].each { |f| field.add_mapping(f[:label], f[:integer_range_set]) }
  end

  # Callbacks
  def callback_create_field_class_integer_unsigned(trace_class, d)
    s = trace_class.create_field_class_integer_unsigned
    populate_integer_field_class(s, d[:class_properties]) if d[:class_properties]
    s
  end
  create_callback_aliases(%w[integer_unsigned unsigned], 'field_class_integer_unsigned')

  def callback_create_field_class_integer_signed(trace_class, d)
    s = trace_class.create_field_class_integer_signed
    populate_integer_field_class(s, d[:class_properties]) if d[:class_properties]
    s
  end
  create_callback_aliases(%w[integer_signed signed], 'field_class_integer_signed')

  def callback_create_structure(trace_class, d)
    s = trace_class.create_structure
    d[:fields].each { |f| s.append(f[:name], callback(trace_class, f)) }
    s
  end

  def callback_create_enumeration_unsigned(trace_class, d)
    s = trace_class.create_unsigned_enumeration_class
    populate_enumeration_class(s, d)
    s
  end

  def callback_create_enumeration_signed(trace_class, d)
    s = trace_class.create_signed_enumeration_class
    populate_enumeration_class(s, d)
    s
  end

  def callback_create_static_array(trace_class, d)
    trace_class.create_dynamic_array(callback(trace_class, d[:field]), d[:length])
  end
  create_callback_aliases(['array_static'], 'static_array')

  def callback_create_dynamic_array(trace_class, d)
    trace_class.create_dynamic_array(callback(trace_class, d[:field]))
  end
  create_callback_aliases(['array_dynamic'], 'dynamic_array')

  def callback_create_string(trace_class, d)
    s = trace_class.create_string
    s.user_attributes = { be_class: d[:be_class] } if d[:be_class]
    s
  end

  def callback_create_bottom(trace_class, d)
    trace_class.send("create_#{d[:class]}")
  end
  bottom_aliases = %w[bool
                      bit_array
                      real_single_precision real_single single single_precision
                      real_double_precision read_double double double_precision]
  create_callback_aliases(bottom_aliases, 'bottom')

  # Callback dispatcher
  def callback(trace_class, d)
    send("callback_create_#{d[:class]}", trace_class, d)
  end

  # Call the callbacks. Start the recursion
  d = { class: 'structure', fields: l }
  callback(trace_class, d)
end


BOTTOM_CLASS_DEFAULT = { BT_FIELD_CLASS_TYPE_SIGNED_INTEGER: 0,
                         BT_FIELD_CLASS_TYPE_BOOL: false,
                         BT_FIELD_CLASS_TYPE_STRING: '' }
BOTTOM_CLASS_DEFAULT.default = 0

downstream_messages = nil

def be_populate_struc_field(struct, field_value)
  field_value.each { |k, v|
    if v.is_a?(Hash)
      be_populate_struc_field(struct[k.to_sym], v)
    else
      struct[k.to_sym] = v
    end
  }
end

def populate_field(field, field_value)
  case field.get_class_type
  when :BT_FIELD_CLASS_TYPE_STRING
    be_class = field.get_class.user_attributes[:be_class]
    n = be_class.nil? ? nil : be_class.value
    if n
      if n.start_with?('ZE')
        thapi_root = ENV['THAPI_ROOT']
        require "/#{thapi_root}/share/ze_bindings"
        struct = ZE.const_get(n.to_sym).new
      elsif n.star_with?('CL')
        struct = OpenCL.const_get(n.to_sym).new
      elsif n.start_with?('CU')
        struct = CU.const_get(n.to_sym).new
      end
      be_populate_struc_field(struct, field_value) if field_value

      value = struct.to_ptr.read_bytes(struct.size)
      field.append(value, length: value.size)
    else
      value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[field.get_class_type] : field_value
      field.set_value(value)
    end
  when :BT_FIELD_CLASS_TYPE_STRUCTURE
    field.field_names.each { |name| populate_field(field[name], field_value.nil? ? nil : field_value[name]) }
  when :BT_FIELD_CLASS_TYPE_STATIC_ARRAY
    element_field_class_type = field.get_class.element_field_class.get_type
    field.length.times { |i| populate_field(field[i], field_value.nil? ? nil : field_value[i]) }
  when :BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITHOUT_LENGTH_FIELD
    element_field_class_type = field.get_class.element_field_class.get_type
    if field_value.nil?
      length = 0
    elsif field_value[:length]
      length = field_value[:length]
    elsif field_value[:values]
      length = field_value[:values].length
    end
    field.length = length
    field.length.times { |i| populate_field(field[i], field_value[:values].nil? ? nil : field_value[:values][i]) }
  # Integers, floating points and enum
  else
    value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[field.get_class_type] : field_value
    field.set_value(value)
  end
end

dust_in_message_iterator_next_method = lambda { |self_message_iterator, capacity|
  raise StopIteration if downstream_messages.empty?

  downstream_message = ->(m) { m.last == 'message' }
  stream_beginning = ->(m) { m.last == 'stream_beginning' }
  stream_end = ->(m) { m.last == 'stream_end' }

  downstream_messages.shift(capacity).filter_map { |m0|
    case m0
    when downstream_message
      stream, event_class, common_context, payload, clock_snapshot_value, = m0

      m = self_message_iterator.create_message_event(
            event_class, stream, clock_snapshot_value: clock_snapshot_value)

      populate_field(m.event.common_context_field, common_context) if m.event.common_context_field
      populate_field(m.event.payload_field, payload)

      m
    when stream_beginning
      stream = m0.first
      self_message_iterator.create_stream_beginning(stream)
    when stream_end
      stream = m0.first
      self_message_iterator.create_stream_end(stream)
    end
  }
}

dust_in_message_iterator_initialize_method = lambda { |self_message_iterator, configuration, port|
}

def create_bt_event_class(bt_trace_class, bt_stream_class, event_class)
  name = event_class[:name]
  bt_event_class = bt_stream_class.create_event_class
  bt_event_class.name = name
  if event_class[:payload]
    bt_event_class.payload_field_class =
      create_datastructure(bt_trace_class, event_class[:payload])
  end
  [name, bt_event_class]
end

def create_bt_stream_class(bt_trace_class, bt_clock_class, stream_class)
  bt_stream_class = bt_trace_class.create_stream_class
  stream_id = stream_class[:name]
  if stream_class[:common_context]
    bt_stream_class.event_common_context_field_class =
      create_datastructure(bt_trace_class, stream_class[:common_context])
  end
  if stream_class[:clock_snapshot_value]
    bt_stream_class.default_clock_class = bt_clock_class
  end
  [stream_id, bt_stream_class]
end

dust_in_initialize_method = lambda { |self_component, _configuration, _params, _data|
  # Should read command line option via babeltrace API
  in_data = YAML.load_file($trace_file)
  schema_in_data = in_data
  $options[:schemas]['default_schema'] = schema_in_data if schema_in_data[:event_classes]

  unless schema_in_data[:stream_classes]
    schema_in_data[:stream_classes] = [{ name: 'default_stream_class' }]
  end

  self_component.add_output_port('op0')
  bt_clock_class = self_component.create_clock_class

  bt_trace_class = self_component.create_trace_class
  bt_trace = bt_trace_class.create_trace

  d_stream_class = schema_in_data[:stream_classes].map { |stream_class|
    create_bt_stream_class(bt_trace_class, bt_clock_class, stream_class)
  }.to_h

  d_event_class = schema_in_data[:stream_classes].collect { |stream_class|
    stream_class_id = stream_class[:name]
    bt_stream_class = d_stream_class[stream_class_id]
    schemas = stream_class[:schemas]
    schemas = ['default_schema'] if !schemas || schemas.empty?
    schemas.collect { |schema_name|
      schema = $options[:schemas][schema_name]
      raise "schema: #{schema_name} was not found" unless schema
      schema[:event_classes].collect { |event_class|
        name, bt_event_class =
          create_bt_event_class(bt_trace_class, bt_stream_class, event_class)
        [[stream_class_id, name], bt_event_class]
      }
    }
  }.flatten(2).to_h

  d_stream = in_data[:streams].map { |stream|
    stream_class_id = stream.fetch(:class, 'default_stream_class')
    bt_stream_class = d_stream_class[stream_class_id]
    [stream[:name], [stream_class_id, stream[:common_context], bt_stream_class.create_stream(bt_trace)]]
  }.to_h

  # Stream begin
  downstream_messages = in_data[:streams].map { |stream|
    stream_id = stream[:name]
    bt_stream = d_stream[stream_id].last
    [bt_stream, 'stream_beginning']
  }

  # Actual Message
  clock_snapshot_values = Hash.new(0)
  downstream_messages += in_data[:events].map { |event|
    name = event[:name]
    stream_id = event[:stream]
    stream_class_id, common_context, stream = d_stream[stream_id]

    event_class = d_event_class[[stream_class_id, name]]
    if stream.get_class.default_clock_class.nil?
      clock_snapshot_value = nil
    elsif event[:clock_snapshot_value]
      clock_snapshot_value = event[:clock_snapshot_value]
      clock_snapshot_values[stream] = event[:clock_snapshot_value] + 1
    else
      clock_snapshot_value = clock_snapshot_values[stream]
      clock_snapshot_values[stream] += 1
    end
    [stream, event_class, common_context, event[:payload], clock_snapshot_value, 'message']
  }

  # Stream end
  downstream_messages += in_data[:streams].map { |stream|
    stream_id = stream[:name]
    bt_stream = d_stream[stream_id].last
    [bt_stream, 'stream_end']
  }

  # self_component.set_data(downstream_messages)
}

# Source Dust
dust_in_message_iterator_class = BT2::BTMessageIteratorClass.new(next_method: dust_in_message_iterator_next_method)
dust_in_message_iterator_class.initialize_method = dust_in_message_iterator_initialize_method

dust_in_class = BT2::BTComponentClass::Source.new(name: 'repeat',
                                                  message_iterator_class: dust_in_message_iterator_class)
dust_in_class.initialize_method = dust_in_initialize_method

# Sink details
sink_text_details = BT2::BTPlugin.find($options[:sink][0]).get_sink_component_class_by_name($options[:sink][1])
# Graph creation

graph = BT2::BTGraph.new

comp1 = graph.add(dust_in_class, 'dust')
comp2 = graph.add(sink_text_details, $options[:sink].join(":"))

op = comp1.output_port(0)
ip = comp2.input_port(0)
graph.connect_ports(op, ip)

graph.run
