require_relative 'gen_opencl_library_base'

# The enum a registry group describes: a value reads back as the constant that
# owns it (constant_value).
def registry_enum(name, values)
  members = values.filter_map do |n, v|
    val = constant_value(v)
    YAMLCAst::Enumerator.new(name: n, val: val) if val
  end
  YAMLCAst::Enum.new(name: name, members: members)
end

print_ffi_module(NAMING)

puts <<~EOF
  module CL
    CL_NAME_VERSION_MAX_NAME_SIZE = 64
    CL_NAME_VERSION_MAX_NAME_SIZE_KHR = 64
    CL_QUEUE_FAMILY_MAX_NAME_SIZE_INTEL = 64
    extend FFI::Library

EOF

# The scalar typedefs, by the size and sign their chain ends in.
API.types.each do |t|
  if INT_SIZE_MAP.key?(t.name)
    puts "  typedef :#{'u' unless INT_SIGN_MAP[t.name]}int#{INT_SIZE_MAP[t.name] * 8}, #{to_ffi_name(t.name)}"
  elsif API.type_classes.floats.include?(t.name)
    puts "  typedef #{to_ffi_name(t.type.name)}, #{to_ffi_name(t.name)}"
  end
end
puts

OPENCL_XML['enums'].each { |name, values| print_enum_with_namespace(NAMING, name, registry_enum(name, values)) }
OPENCL_XML['bitfields'].each do |name, values|
  print_bitfield_with_namespace(NAMING, name, registry_enum(name, values))
end

print_typedefs(NAMING, enum: nil, function_pointer: nil, integer: nil)

# The keys of a device-partition property list that take a list of values, and
# the constant that ends it: what the tracer walks the list with.
puts '  PARTITION_VALUE_LISTS = {'
DeviceFissionPropertyList::VALUE_LISTS.each { |type, lists| puts "    #{NAMING.class_name(type)} => #{lists}," }
puts '  }.freeze'

puts 'end'
