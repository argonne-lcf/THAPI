#!/usr/bin/env ruby
# Disclaimer: This script has been only tested on HIP model.

require 'yaml'

HIP_STRUCT_TYPES = [
  'struct hipChannelFormatDesc',
  'hipIpcMemHandle_t',
  'hipIpcEventHandle_t',
  'hipPitchedPtr',
  'hipExtent',
  'hipPitchedPtr',
  'hipExtent',
  'struct hipExtent',
  'dim3',
  'hipPitchedPtr',
  'hipExtent',
  'hipPitchedPtr',
  'hipExtent',
  'dim3',
]

PROPERTIES_DICT = {
  :class => :type
}

DATA_TYPES_DICT = {
  'unsigned' => 'integer_unsigned',
  'signed' => 'integer_signed',
}

# Not supported in the metababel model
INGONRE_PROPERTIES = [
  :length, 
  :be_class,
]

def translate_key(key)
  return key unless PROPERTIES_DICT.key?(key)
  PROPERTIES_DICT[key]
end

def translate_value(value)
  return value unless DATA_TYPES_DICT.key?(value)
  DATA_TYPES_DICT[value]
end

def translate_key_value(k,v)
  [translate_key(k), translate_value(v)]
end

def get_element_field_class(element_field, parent_field)
  class_properties = element_field.delete(:class_properties)
  field_properties = (element_field.to_a + class_properties.to_a).filter_map {|k,v| translate_key_value(k,v) unless INGONRE_PROPERTIES.include?(k) }.to_h
  if parent_field[:class] == 'array_dynamic'
    match = parent_field[:cast_type].match(/(.*) \*/)
    raise ":cast_type for element_field in array_dynamic can not be determined for #{parent_field}." unless match
    field_properties[:cast_type] = match[1]
  end

  { :element_field_class => field_properties }
end

def get_field_class_properties(field, member_name)
  class_properties = field.delete(:class_properties)
  field_element_field_class = field.key?(:field) ? get_element_field_class(field.delete(:field), field) : {}
  field_properties = (field.to_a + class_properties.to_a + field_element_field_class.to_a ).filter_map {|k,v| translate_key_value(k,v) unless INGONRE_PROPERTIES.include?(k) }.to_h
  field_properties[:cast_type_is_struct] = true if HIP_STRUCT_TYPES.include?(field[:cast_type])
  field_properties[:length_field_path] = "EVENT_PAYLOAD[\"_#{member_name}_length\"]" if field_properties[:type] == 'array_dynamic'
  
  field_properties
end

def get_event_class_member(member)
  member_name = member.delete(:name)
  { :name => member_name, :field_class => get_field_class_properties(member, member_name) }
end

def get_event_class(evt)
  members = evt[:payload].map(&method(:get_event_class_member))
  event_class = { :name => evt.delete(:name) }
  event_class[:payload_field_class] = { :type => 'structure', :members =>  members } unless members.empty?
  event_class
end

def get_stream_class(sc)
  {
    :name => sc[:name],
    :default_clock_class => {},
    :packet_context_field_class => {
      :type => 'structure',
      :members => [
        {
          :name => 'cpu_id',
          :field_class => {
            :type => 'integer_unsigned',
            :field_value_range => 32,
            :cast_type => 'uint64_t'
          }
        }
      ]
    },
    :event_common_context_field_class => {
      :type => 'structure',
      :members => [
        {
          :name => 'vpid',
          :field_class => {
            :type => 'integer_signed',
            :field_value_range => 64,
            :cast_type => 'int64_t'
          }
        },
        {
          :name => 'vtid',
          :field_class => {
            :type => 'integer_unsigned',
            :field_value_range => 64,
            :cast_type => 'uint64_t'
          }
        }
      ]
    },
    :event_classes => sc[:event_classes].map(&method(:get_event_class))
  }
end

raise "Not input model provided" unless ARGV.length > 0
input_model_path = ARGV[0] 
model = YAML.load_file(input_model_path)

trace = { 
  :environment => {
    :entries => [{ :name => 'hostname', :type => 'string' }]
  },
  :stream_classes => [get_stream_class(model)],
}.to_yaml

puts trace
