require_relative 'gen_opencl_library_base'
require_relative '../../utils/gen_babeltrace_lib_helper'

# The unpack directive of a scalar type, from its size and sign.
def unpack_directive(type)
  return { 'cl_float' => 'f', 'cl_double' => 'd' }.fetch(type) if API.type_classes.floats.include?(type)

  directive = { 1 => 'c', 2 => 's', 4 => 'l', 8 => 'q' }.fetch(INT_SIZE_MAP.fetch(type))
  INT_SIGN_MAP.fetch(type) ? directive : directive.upcase
end

# How to read what a clGet*Info call returns for one key (opencl_infos.yaml):
# see CL.info_to_s.
def info_reader(info)
  type = info['type']
  named = NAMING.scoped_class_name(type) if (API.enum_names + API.bitfield_names + API.struct_names).include?(type)
  format = if info['string'] || API.struct_names.include?(type) then 'Z*'
           elsif info['pointer'] || API.object?(type) || type == 'uintptr_t' then 'J'
           else unpack_directive(type)
           end
  format += '*' if info['array'] && format != 'Z*'
  reader = [format.inspect, !!info['array'], named || 'nil']
  reader << ':partition_list' if info['name'] == 'CL_DEVICE_PARTITION_TYPE'
  "[#{reader.join(', ')}]"
end

puts "require_relative 'babeltrace_opencl_values.rb'"
# By the value of the key: the registry gives some keys two names
# (CL_PROGRAM_IL, CL_PROGRAM_IL_KHR), and a value reads back as only one.
puts 'CL::INFOS = {'
yaml_load_file_cached(File.join(SRC_DIR, 'opencl_infos.yaml')).each do |type, versions|
  keys = OPENCL_XML['enums'].fetch(type, {})
  readers = versions.values.flatten.to_h { |i| [constant_value(keys[i['name']]), "#{info_reader(i)}, # #{i['name']}"] }
  puts "  #{type.inspect} => {"
  readers.each { |value, reader| puts "    #{value} => #{reader}" if value }
  puts '  },'
end
puts '}.freeze'

# The fields whose type does not say how to print them.
render = lambda do |event, field|
  name = field[:name]
  type = field[:field_class][:cast_type]
  if name == 'param_value_vals'
    key = event[:payload_field_class][:members].find { |m| %w[_param_name param_name].include?(m[:name]) }
    next unless key

    infos = %(CL::INFOS.fetch("#{key[:field_class][:cast_type]}", {}))
    %(s << "#{name}: \#{CL.info_to_s(#{infos}, defi["#{key[:name]}"], defi["#{name}"])}")
  elsif type.match?(/\Acl_device_partition_property(_ext)? \*\z/) && field[:field_class][:type] == 'array_dynamic'
    list = NAMING.scoped_class_name(type.delete_suffix(' *'))
    %(s << "#{name}: \#{CL::PARTITION_LISTS[#{list}].to_a(defi["#{name}"])}")
  elsif name == 'errcode_ret_val'
    %(s << "#{name}: \#{CL::Errcode.from_native(defi["#{name}"], nil)}")
  end
end

print_babeltrace_lib(NAMING, render: render)
