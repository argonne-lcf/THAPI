require 'delegate'
require 'babeltrace2'
require 'yaml'

thapi_root = ENV["THAPI_ROOT"]
require "/#{thapi_root}/share/ze_bindings"

module FooExtensions
  attr_accessor :be_class
end

class BT2::BTFieldClass::String
  prepend FooExtensions
end


def create_datastructure(trace_class, l)

  def create_callback_aliases(aliases, root)
    aliases.each { |new|
      singleton_class.send(:alias_method, "callback_create_#{new}", "callback_create_#{root}")
  } 
  end 

  def populate_integer_field_class(field, d)
    field.preferred_display_base = d[:preferred_display_base] if d[:preferred_display_base]
    field.field_value_range = sd[:field_value_range] if sd[:field_value_range]
  end 

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

  def callback_create_static_array(trace_class, d)
    trace_class.create_static_array(callback(trace_class,d[:field]), d[:length])
  end

  def callback_create_string(trace_class, d)
    s = trace_class.create_string
    s.user_attributes = { be_class: d[:be_class] }
    s
  end

  def callback_create_bottom(trace_class, d)
    trace_class.send("create_#{d[:class]}")
  end
  bottom_aliases = ['bool',
   'bit_array',
   'enumeration_unsigned',
   'enumeration_signed',
   'real_single_precision','real_single', 'single','single_precision',
   'real_double_precision','read_double', 'double','double_precision']
  create_callback_aliases(bottom_aliases, "bottom") 

  def callback(trace_class, d)
    send("callback_create_#{d[:class]}",trace_class, d)
  end 
  
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
    
    c = field.get_class_type
    if c == :BT_FIELD_CLASS_TYPE_STRING
      n = field.get_class.user_attributes[:be_class].value
      if n
        if n.start_with?("ZE")
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
        value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[c] : field_value
        field.set_value(value)
      end 
    elsif c == :BT_FIELD_CLASS_TYPE_STRUCTURE
      field.field_names.each { |name| populate_field(field[name], field_value.nil? ? nil :  field_value[name] ) }
    elsif c ==:BT_FIELD_CLASS_TYPE_STATIC_ARRAY 
      value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[field.get_element_field_class_type] : field_value
      field.length.times{ |i| populate_field(field[i], field_value.nil? ? nil :  field_value[i] ) }
    else
      value = field_value.nil? ? BOTTOM_CLASS_DEFAULT[c] : field_value 
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

      populate_field(m.event.common_context_field, common_context)  
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
 

  self_component.add_output_port('op0')
  clock_class = self_component.create_clock_class 

  trace_class = self_component.create_trace_class 
  trace = trace_class.create_trace

  d_stream_class = in_data[:schema_streams].map { |schema|
    stream_class = trace_class.create_stream_class
    stream_id = schema[:name]

    stream_class.event_common_context_field_class = create_datastructure(trace_class, schema[:common_context])
    stream_class.default_clock_class = clock_class if schema[:clock_snapshot_value]

    [stream_id, stream_class] #, stream_class.create_stream(trace)] ]
  }.to_h

  d_event = in_data[:schema_events].map { |schema|
    stream_class_id = schema[:stream]
    stream_class = d_stream_class[stream_class_id]

    name = schema[:name]
    event_class = stream_class.create_event_class
    event_class.name = name
    event_class.payload_field_class = create_datastructure(trace_class, schema[:payload])

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
