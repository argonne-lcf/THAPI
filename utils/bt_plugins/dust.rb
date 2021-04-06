require 'babeltrace2'
require 'yaml'
require 'find'

def create_datastructure(trace_class, l)
  # Utils functions
  def create_callback_aliases(aliases, root)
    aliases.each do |new|
      singleton_class.send(:alias_method, "callback_create_#{new}", "callback_create_#{root}")
    end
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
    trace_class.create_static_array(callback(trace_class, d[:field]), d[:length])
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

def be_populate_struc_field(struct, field_value)
  field_value.each do |k, v|
    if v.is_a?(Hash)
      be_populate_struc_field(struct[k.to_sym], v)
    else
      struct[k.to_sym] = v
    end
  end
end

def populate_field(field, field_value)
  case field.get_class_type
  when :BT_FIELD_CLASS_TYPE_STRING
    be_class = field.get_class.user_attributes[:be_class]
    n = be_class.nil? ? nil : be_class.value
    if n
      if n.start_with?('ZE::')
        require "#{ENV["BINDING_DIR"]}/ze_library"
        struct = eval(n).new
      elsif n.start_with?('CUDA::')
        require "#{ENV["BINDING_DIR"]}/cuda_library"
        struct = eval(n).new
      elsif n.start_with?('CL::')
        require 'opencl_ruby_ffi/opencl_types'
        require 'opencl_ruby_ffi/opencl_arithmetic_gen'
        require 'opencl_ruby_ffi/opencl_ruby_ffi_base_gen'
        require 'opencl_ruby_ffi/opencl_ruby_ffi_base'
        struct = OpenCL.const_get(n[4..-1].to_sym).new
      elsif n.start_with?('CU')
        struct = CU.const_get(n.to_sym).new
      else
        raise "unsupported be_class structure #{n}"
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
      length = field_value[:dust]
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
  bt_stream_class.default_clock_class = bt_clock_class if stream_class[:clock_snapshot_value]
  [stream_id, bt_stream_class]
end

def find_file_in_envfolder(str, target)
    r = Find.find(*str.split(':')).find { |f| File.file?(f) && File.basename(f) == target }
    raise "Cannot find #{target} in #{str}" if r.nil?
    r
end

class Dust
  def initialize(trace = nil, schemas = nil)
    @trace = trace
    @schemas = schemas
  end

  def message_iterator_next_method(self_message_iterator, capacity)
    raise StopIteration if @downstream_messages.empty?

    downstream_message = ->(m) { m.last == 'message' }
    stream_beginning = ->(m) { m.last == 'stream_beginning' }
    stream_end = ->(m) { m.last == 'stream_end' }

    @downstream_messages.shift(capacity).filter_map do |m0|
      case m0
      when downstream_message
        stream, event_class, common_context, payload, clock_snapshot_value, = m0

        m = self_message_iterator.create_message_event(
          event_class, stream, clock_snapshot_value: clock_snapshot_value
        )

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
    end
  end

  def initialize_method(self_component, _configuration, params, _data)
    trace = params.get_entry_value('trace') ? params.get_entry_value('trace').value : @trace
    schemas = params.get_entry_value('schemas') ? params.get_entry_value('schemas').value : @schemas
    @in_data = YAML.load_file(find_file_in_envfolder(ENV['DUST_TRACE_DIR'],trace))
    @schemas = schemas.map do |path|
      schema = YAML.load_file(find_file_in_envfolder(ENV['DUST_MODELS_DIR'],path))
      [schema[:name], schema]
    end.to_h
    # Should read command line option via babeltrace API
    schema_in_data = @in_data
    @schemas['default_schema'] = schema_in_data if schema_in_data[:event_classes]
    @schemas.merge!(@in_data[:schemas]) if @in_data[:schemas]
    
    schema_in_data[:stream_classes] = [{ name: 'default_stream_class' }] unless schema_in_data[:stream_classes]

    self_component.add_output_port('op0')
    bt_clock_class = self_component.create_clock_class

    bt_trace_class = self_component.create_trace_class
    bt_trace = bt_trace_class.create_trace
    bt_trace.set_environment_entry_string('hostname', @in_data[:hostname]) if @in_data[:hostname]

    d_stream_class = schema_in_data[:stream_classes].map do |stream_class|
      create_bt_stream_class(bt_trace_class, bt_clock_class, stream_class)
    end.to_h

    d_event_class = schema_in_data[:stream_classes].collect do |stream_class|
      stream_class_id = stream_class[:name]
      bt_stream_class = d_stream_class[stream_class_id]
      schemas = stream_class[:schemas]
      schemas = ['default_schema'] if !schemas || schemas.empty?
      schemas.collect do |schema_name|
        schema = @schemas[schema_name]
        raise "schema: #{schema_name} was not found" unless schema
        schema[:event_classes].collect do |event_class|
          name, bt_event_class =
            create_bt_event_class(bt_trace_class, bt_stream_class, event_class)
          [[stream_class_id, name], bt_event_class]
        end
      end
    end.flatten(2).to_h

    d_stream = @in_data[:streams].map do |stream|
      stream_class_id = stream.fetch(:class, 'default_stream_class')
      bt_stream_class = d_stream_class[stream_class_id]
      [stream[:name], [stream_class_id, stream[:common_context], bt_stream_class.create_stream(bt_trace)]]
    end.to_h

    # Stream begin
    @downstream_messages = @in_data[:streams].map do |stream|
      stream_id = stream[:name]
      bt_stream = d_stream[stream_id].last
      [bt_stream, 'stream_beginning']
    end

    # Actual Message
    clock_snapshot_values = Hash.new(0)
    @downstream_messages += @in_data[:events].map do |event|
      name = event[:name]
      stream_id = event[:stream]
      stream_id ||= @in_data[:default_stream]
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
      if event[:common_context]
        common_context = if common_context
                           common_context.merge(event[:common_context])
                         else
                           event[:common_context]
                         end
      end
      [stream, event_class, common_context, event[:payload], clock_snapshot_value, 'message']
    end

    # Stream end
    @downstream_messages += @in_data[:streams].map do |stream|
      stream_id = stream[:name]
      bt_stream = d_stream[stream_id].last
      [bt_stream, 'stream_end']
    end
  end

  def create_component_class
    message_iterator_class = BT2::BTMessageIteratorClass.new(next_method: lambda(&method(:message_iterator_next_method)))
    component_class = BT2::BTComponentClass::Source.new(name: 'dust',
                                                        message_iterator_class: message_iterator_class)
    component_class.initialize_method = lambda(&method(:initialize_method))
    component_class
  end
end
