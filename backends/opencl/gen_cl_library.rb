require_relative 'gen_opencl_library_base'

# The registry writes a constant's value as a literal or a bit (`(1 << n)`).
def constant_value(str)
  Integer(str, exception: false) || (1 << Integer(Regexp.last_match(1)) if str.match(/\A\(1 << (\d+)\)\z/))
end

# The enum a registry group describes. A constant with no value of its own is
# reserved, or another name for a constant of the group (CL_BLOCKING is
# CL_TRUE): a value reads back as the constant that owns it.
def registry_enum(name, group)
  members = group['values'].filter_map do |n, v|
    YAMLCAst::Enumerator.new(name: n, val: constant_value(v)) if v && constant_value(v)
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

OpenCLApi::GROUPS['enums'].each { |name, g| print_enum_with_namespace(NAMING, name, registry_enum(name, g)) }
OpenCLApi::GROUPS['bitfields'].each { |name, g| print_bitfield_with_namespace(NAMING, name, registry_enum(name, g)) }

print_typedefs(NAMING, enum: nil, function_pointer: nil, integer: nil)

puts 'end'
