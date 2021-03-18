require 'babeltrace2'
require 'yaml'
require 'optparse'

class Comparator
  def consume_method(_self_component)
    # This function will comsume message for all the message iterator
    # and compare events fields
    # If the number of message consumed by message iterator are differnts
    # an error is raised
    stack_messages = [[], []]

    @message_iterators.each_with_index { |message_iterator, i|
      message_iterator.next_messages.each { |m|
        stack_messages[i] << m.event if m.type == :BT_MESSAGE_TYPE_EVENT
      }
    }

    stack_messages.transpose.each { |i, j|
      %w[payload specific_context common_context].each { |tf|
        i_value = i.send("get_#{tf}_field")
        i_value = i_value.value if i_value
        j_value = j.send("get_#{tf}_field")
        j_value = j_value.value if j_value
        unless i_value == j_value
          pp({ in0: i_value, in1: j_value })
          raise "Traces for #{tf} fields are different!"
        end
      }
    }
  end

  def initialize_method(self_component, _configuration, _params, _data)
    self_component.add_input_port('in0')
    self_component.add_input_port('in1')
  end

  def graph_is_configured_method(self_component)
    @message_iterators = self_component.get_input_port_count.times.map { |i|
      p = self_component.get_input_port_by_index(i)
      self_component.create_message_iterator(p)
    }
  end

  def create_component_class
    component_class = BT2::BTComponentClass::Sink.new(name: 'comparator',
                                                      consume_method: lambda(&method(:consume_method)))

    component_class.initialize_method = lambda(&method(:initialize_method))
    component_class.graph_is_configured_method = lambda(&method(:graph_is_configured_method))
    component_class
  end
end
