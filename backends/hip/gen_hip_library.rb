require_relative 'gen_hip_library_base.rb'

def print_enum(name, enum)
  print_enum_with_namespace(:HIP, name, enum)
end

def print_hip_object(object)
  print_object(object)
end

print_ffi_module(:HIP)

puts <<EOF
module HIP
  HIP_IPC_HANDLE_SIZE = 64
  extend FFI::Library

  module Handle
    def to_s
      s = '{ reserved: "'
      s << self[:reserved].to_a.collect { |v| "\\\\x%02x" % ((v + 256)%256) }.join
      s << '" }'
    end
  end

  module UUID
    def to_s
      a = self[:bytes].to_a.collect { |v| v < 0 ? 0x100 + v : v }
      s = "{ id: "
      s << "%02x" % a[0]
      s << "%02x" % a[1]
      s << "%02x" % a[2]
      s << "%02x" % a[3]
      s << "-"
      s << "%02x" % a[4]
      s << "%02x" % a[5]
      s << "-"
      s << "%02x" % a[6]
      s << "%02x" % a[7]
      s << "-"
      s << "%02x" % a[8]
      s << "%02x" % a[9]
      s << "-"
      s << "%02x" % a[10]
      s << "%02x" % a[11]
      s << "%02x" % a[12]
      s << "%02x" % a[13]
      s << "%02x" % a[14]
      s << "%02x" % a[15]
      s << " }"
    end
  end

  hipDeviceAttributeCudaCompatibleBegin = 0
  hipDeviceAttributeAmdSpecificBegin = 10000
  HIPRTC_JIT_NUM_LEGACY_INPUT_TYPES = 6
  ACTIVITY_DOMAIN_HIP_OPS = 2
EOF

def print_union(name, union)
  print_union_with_namespace(:HIP, name, union)
end

def print_struct(name, struct)
  prepends = []
  prepends << "UUID" if to_class_name(name).match("UUID")
  print_struct_with_namespace(:HIP, name, struct, prepends: prepends)
end

$all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  elsif $objects.include?(t.name)
    print_hip_object(t.name)
  elsif t.type.kind_of? YAMLCAst::Struct
    struct = $all_structs.find { |s| t.type.name == s.name }
    next unless struct
    print_struct(t.name, struct)
  elsif t.type.kind_of? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union
    print_union(t.name, union)
  elsif t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  elsif t.type.kind_of?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.kind_of?(YAMLCAst::Int)
    print_int_type(t.name, t.type.name)
  else
    #$stderr.puts t.inspect
  end
}

puts <<EOF
end
EOF
