require 'babeltrace2'
require 'yaml'
require 'optparse'

class Comparator
  def consume_method(_self_component)
    # This function will consume message for all the message iterator
    @message_iterators.delete_if do |message_iterator|
      message_iterator.next_messages.each do |m|
        @stack_messages[message_iterator] << m.event if m.type == :BT_MESSAGE_TYPE_EVENT
      end
    rescue StopIteration
      true
    else
      false
    end

    return unless @message_iterators.empty?

    verify_messages
    @stack_messages = nil
    raise StopIteration
  end

  def initialize_method(self_component, _configuration, _params, _data)
    @stack_messages = Hash.new{ |h,k| h[k] = [] }
    self_component.add_input_port('in0')
    self_component.add_input_port('in1')
  end

  def verify_messages
    if @stack_messages.values.uniq{ |data| data.length }.length > 1
      raise "Traces have different number of messages"
    end
    @stack_messages.values.transpose.each do |i, j|
      if i.name != j.name
        raise "Trace messages have different names: #{i.name} != #{j.name}"
      end
      %w[payload specific_context common_context].each do |tf|
        i_value = i.send("get_#{tf}_field")
        i_value = i_value.value if i_value
        j_value = j.send("get_#{tf}_field")
        j_value = j_value.value if j_value
        unless i_value == j_value
          i_hash = {}
          j_hash = {}
          %w[payload specific_context common_context].each do |tf|
            i_value = i.send("get_#{tf}_field")
            i_value = i_value.value if i_value
            j_value = j.send("get_#{tf}_field")
            j_value = j_value.value if j_value
            i_hash[tf] = i_value
            j_hash[tf] = j_value
          end
          pp({ in0: i_hash, in1: j_hash })
          raise "Traces for event #{i.name}, #{tf} fields are different!"
        end
      end
    end
  end

  def graph_is_configured_method(self_component)
    @message_iterators = self_component.get_input_port_count.times.map do |i|
      p = self_component.get_input_port_by_index(i)
      self_component.create_message_iterator(p)
    end
  end

  def create_component_class
    component_class = BT2::BTComponentClass::Sink.new(name: 'comparator',
                                                      consume_method: lambda(&method(:consume_method)))

    component_class.initialize_method = lambda(&method(:initialize_method))
    component_class.graph_is_configured_method = lambda(&method(:graph_is_configured_method))
    component_class
  end
end
