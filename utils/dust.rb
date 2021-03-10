require 'babeltrace2'
require 'yaml'

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
   
  def populate_enumeration_class(field,d)
    populate_integer_field_class(field,d)
    d[:mapping].each{ |f| field.add_mapping(f[:label],  f[:integer_range_set]) }
  end

  # Callbacks
  def callback_create_field_class_integer_unsigned(trace_class, d)
    s = trace_class.create_field_class_integer_unsigned
    populate_integer_field_class(s, d[:class_properties]) if d[:class_properties]
    s
  end
  create_callback_aliases(['integer_unsigned','unsigned'], 'field_class_integer_unsigned')
 
  def callback_create_field_class_integer_signed(trace_class, d)
    s = trace_class.create_field_class_integer_signed
    populate_integer_field_class(s, d[:class_properties]) if d[:class_properties]
    s
  end
  create_callback_aliases(['integer_signed','signed'], 'field_class_integer_signed')

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
    trace_class.create_dynamic_array(callback(trace_class,d[:field]), d[:length])
  end
  create_callback_aliases(['array_static'], 'static_array')

  def callback_create_dynamic_array(trace_class, d)
    trace_class.create_dynamic_array(callback(trace_class,d[:field]))
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
  bottom_aliases = ['bool',
   'bit_array',
   'real_single_precision','real_single', 'single','single_precision',
   'real_double_precision','read_double', 'double','double_precision']
  create_callback_aliases(bottom_aliases, "bottom") 
  
  # Callback dispatcher
  def callback(trace_class, d)
    send("callback_create_#{d[:class]}",trace_class, d)
  end 

  # Call the callbacks. Start the recursion  
  d = {:class => "structure", :fields => l}
  callback(trace_class, d)

end

USR_DATA_LOCATION = ARGV[0]


BOTTOM_CLASS_DEFAULT = {BT_FIELD_CLASS_TYPE_SIGNED_INTEGER: 0, 
                        BT_FIELD_CLASS_TYPE_BOOL: false,
                        BT_FIELD_CLASS_TYPE_STRING: "" }
BOTTOM_CLASS_DEFAULT.default = 0

downstream_messages = nil

def be_populate_struc_field(struct, field_value)
  field_value.each { |k, v|
    if v.kind_of?(Hash)
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
        if n.start_with?("ZE")
          thapi_root = ENV["THAPI_ROOT"]
          require "/#{thapi_root}/share/ze_bindings"
          struct = ZE.const_get(n.to_sym).new()
        elsif n.star_with?("CL")
          struct = OpenCL.const_get(n.to_sym).new()
        elsif n.start_with?("CU")
          struct = CU.const_get(n.to_sym).new()
        end
        be_populate_struc_field(struct, field_value) if field_value

        value = struct.to_ptr.read_bytes(struct.size)
        field.append(value, length: value.size)
      else
        value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[field.get_class_type] : field_value
        field.set_value(value)
      end 
    when :BT_FIELD_CLASS_TYPE_STRUCTURE
      field.field_names.each { |name| populate_field(field[name], field_value.nil? ? nil :  field_value[name] ) }
    when :BT_FIELD_CLASS_TYPE_STATIC_ARRAY
      element_field_class_type = field.get_class.element_field_class.get_type 
      field.length.times{ |i| populate_field(field[i], field_value.nil? ? nil :  field_value[i] ) }
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
       field.length.times{ |i| populate_field(field[i], field_value[:values].nil? ? nil :  field_value[:values][i] ) }
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
      
      m = self_message_iterator.create_message_event(event_class, stream, 
                                                     clock_snapshot_value: clock_snapshot_value) 

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

dust_in_initialize_method = lambda { |self_component, _configuration, _params, _data|
  # Should read command line option via babeltrace API
  in_data = YAML.load_file(USR_DATA_LOCATION)
  schema_in_data =  in_data[:schema_path] ? YAML.load_file(in_data[:schema_path]): in_data

  self_component.add_output_port('op0')
  clock_class = self_component.create_clock_class 

  trace_class = self_component.create_trace_class 
  trace = trace_class.create_trace

  d_stream_class = schema_in_data[:schema_streams].map { |schema|
    stream_class = trace_class.create_stream_class
    stream_id = schema[:name]

    stream_class.event_common_context_field_class = create_datastructure(trace_class, schema[:common_context]) if schema[:common_context]
    stream_class.default_clock_class = clock_class if schema[:clock_snapshot_value]
    [stream_id, stream_class]
  }.to_h

  d_event = schema_in_data[:schema_events].map { |schema|
    stream_class_id = schema[:stream]
    stream_class = d_stream_class[stream_class_id]

    name = schema[:name]
    event_class = stream_class.create_event_class
    event_class.name = name
    event_class.payload_field_class = create_datastructure(trace_class, schema[:payload]) if schema[:payload]
    [[stream_class_id, name], event_class]
  }.to_h

  d_stream = in_data[:streams].map { |fields|
     stream_class_id  = fields[:class]
     stream_class = d_stream_class[stream_class_id]
     [fields[:name], [stream_class_id, fields[:common_context], stream_class.create_stream(trace) ] ] 
  }.to_h

  # Stream begin
  downstream_messages = in_data[:streams].map { |fields|
    stream_id = fields[:name]
    stream = d_stream[stream_id].last
    [stream, 'stream_beginning']
  }

  # Actual Message
  clock_snapshot_values = Hash.new(0)
  downstream_messages += in_data[:events].map { |fields|
    name = fields[:name]
    stream_id = fields[:stream]
    stream_class_id, common_context, stream = d_stream[stream_id]

    event_class = d_event[[stream_class_id, name]]
    if stream.get_class.default_clock_class.nil?
       clock_snapshot_value = nil
    elsif fields[:clock_snapshot_value]
       clock_snapshot_value = fields[:clock_snapshot_value]
       clock_snapshot_values[stream] = fields[:clock_snapshot_value]+1
    else
       clock_snapshot_value = clock_snapshot_values[stream]
       clock_snapshot_values[stream]+=1
    end 
    [stream, event_class, common_context, fields[:payload], clock_snapshot_value, 'message']
  }

  # Stream end
  downstream_messages += in_data[:streams].map { |fields|
    stream_id = fields[:name]
    stream = d_stream[stream_id].last
    [stream, 'stream_end']
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
sink_text_details = BT2::BTPlugin.find('text').get_sink_component_class_by_name('details')
# Graph creation

graph = BT2::BTGraph.new

comp1 = graph.add(dust_in_class, 'dust')
comp2 = graph.add(sink_text_details, 'pretty')

op = comp1.output_port(0)
ip = comp2.input_port(0)
graph.connect_ports(op, ip)

graph.run
