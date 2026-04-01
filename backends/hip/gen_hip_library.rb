require_relative 'gen_hip_library_base'

def print_enum(name, enum)
  print_enum_with_namespace(:HIP, name, enum)
end

def print_hip_object(object)
  print_object(object)
end

print_ffi_module(:HIP)

puts <<~EOF
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
  prepends << 'UUID' if to_class_name(name).match('UUID')
  print_struct_with_namespace(:HIP, name, struct, prepends: prepends)
end

typedef_enum_names = $all_types.filter_map { |t| t.type.name if t.type.is_a?(YAMLCAst::Enum) }.to_set
$all_enums.each do |e|
  next unless e.name
  next if typedef_enum_names.include?(e.name)

  print_enum(e.name, e)
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  elsif $objects.include?(t.name)
    print_hip_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = t.type.name ? $all_structs.find { |s| t.type.name == s.name } : t.type
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union(t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  elsif t.type.is_a?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.is_a?(YAMLCAst::Int)
    print_int_type(t.name, t.type.name)
  else
    # $stderr.puts t.inspect
  end
end

puts <<~EOF
  end
EOF
