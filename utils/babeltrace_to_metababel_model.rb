#!/usr/bin/env ruby
# Disclaimer: This script has been only tested on HIP, and ZE model.

require 'yaml'

raise "Not input model provided" unless ARGV.length > 0
input_model_path = ARGV[0]
model = YAML.load_file(input_model_path)

# Just look in the payload, good enough for now
STRUCT_TYPES =  model[:event_classes].map { |f|
  f[:payload].filter_map { |l|
    native = l[:class]
    cast = l[:cast_type]

    cast if native == "string" and cast != "char *"  and !cast.include?("*")
  }
}.flatten.uniq

PROPERTIES_DICT = {
  :class => :type
}

DATA_TYPES_DICT = {
  'unsigned' => 'integer_unsigned',
  'signed' => 'integer_signed',
}

# Not supported in the metababel model
INGONRE_PROPERTIES = [
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
    # get array elements' cast_type
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
  field_properties[:length_field_path] = "EVENT_PAYLOAD[\"_#{member_name}_length\"]" if field_properties[:type] == 'array_dynamic'
  # Doesn't know why old model need the size of string.
  # In metababel we always compute is as "sizeof((cast_type)#{var})".
  # It look like ot be same same in the cuda model
  field_properties.delete(:length) if field_properties[:type] == 'string'

  # Special casting case for struct <-> string.
  field_properties[:cast_type_is_struct] = true if STRUCT_TYPES.include?(field[:cast_type])

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
  stream_class = { :name => sc[:name] }

  if sc.key?(:clock_snapshot_value)
    stream_class[:default_clock_class] = {}
  end

  if sc.key?(:packet_context)
    members = sc[:packet_context].map(&method(:get_event_class_member))
    stream_class[:packet_context_field_class] = {
      :type => 'structure',
      :members =>  members
    }
  end

  if sc.key?(:common_context)
    members = sc[:common_context].map(&method(:get_event_class_member))
    stream_class[:event_common_context_field_class] = {
      :type => 'structure',
      :members =>  members
    }
  end

  stream_class[:event_classes] = sc[:event_classes].map(&method(:get_event_class))

  stream_class
end

def get_environment_entry(entry)
  { :name => entry[:name], :type => entry[:class] }
end

# THAPI interval and BTX model differ.
if model.key?(:stream_classes)
  stream_class = model.delete(:stream_classes).pop
  event_classes = model.delete(:event_classes)
  stream_class[:event_classes] = event_classes
else
  stream_class = model
end

trace = {}
environment = model.delete(:environment)
trace[:environment] = { :entries => environment.map(&method(:get_environment_entry)) } if environment
trace[:stream_classes] = [get_stream_class(stream_class)]

puts trace.to_yaml
