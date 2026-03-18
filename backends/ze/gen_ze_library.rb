require_relative 'gen_ze_library_base'

def ZE_BIT(i)
  1 << i
end

def print_version_enum_struct(name)
  puts <<EOF
  class #{to_class_name(name)} < FFI::ZEStruct
    prepend Version
    layout :minor, :uint16,
           :major, :uint16

    def self.from_native(i, ctx = nil)
      v = self::new
      v[:major] = ZE.ZE_MAJOR_VERSION(i)
      v[:minor] = ZE.ZE_MINOR_VERSION(i)
      v
    end
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

def print_enum(name, enum)
  return print_version_enum_struct(name) if name.match(/_version_t\z/)

  if enum.members.find { |m| m.val && m.val.is_a?(String) && m.val.match('ZE_BIT') } || enum.members.all? do |m|
    m.name.include?('_FLAG_')
  end
    print_bitfield_with_namespace(:ZE, name, enum, check_flags: true)
  else
    print_enum_with_namespace(:ZE, name, enum)
  end
end

def print_ze_object(object)
  print_object(object)
end

print_ffi_module(:ZE, enclosing_module: false)

puts <<~EOF
  module ZE
    ZE_MAX_IPC_HANDLE_SIZE = 64
    ZE_MAX_UUID_SIZE = 16
    ZE_MAX_DRIVER_UUID_SIZE = 16
    ZE_MAX_EXTENSION_NAME = 256
    ZE_MAX_DEVICE_UUID_SIZE = 16
    ZE_MAX_DEVICE_NAME = 256
    ZE_SUBGROUPSIZE_COUNT = 8
    ZE_MAX_NATIVE_KERNEL_UUID_SIZE = 16
    ZE_MAX_KERNEL_UUID_SIZE = 16
    ZE_MAX_MODULE_UUID_SIZE = 16
    ZE_MAX_DEVICE_LUID_SIZE_EXT = 8
    ZE_MAX_FABRIC_EDGE_MODEL_EXP_SIZE = 256
    ZES_MAX_EXTENSION_NAME = 256
    ZES_STRING_PROPERTY_SIZE = 64
    ZES_MAX_UUID_SIZE = 16
    ZES_MAX_FABRIC_PORT_MODEL_SIZE = 256
    ZES_MAX_FABRIC_LINK_TYPE_SIZE = 256
    ZES_FAN_TEMP_SPEED_PAIR_COUNT = 32
    ZES_MAX_RAS_ERROR_CATEGORY_COUNT = 7
    ZET_MAX_METRIC_GROUP_NAME = 256
    ZET_MAX_METRIC_GROUP_DESCRIPTION = 256
    ZET_MAX_METRIC_NAME = 256
    ZET_MAX_METRIC_DESCRIPTION = 256
    ZET_MAX_METRIC_COMPONENT = 256
    ZET_MAX_METRIC_RESULT_UNITS = 256
    ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_NAME_EXP = 256
    ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_DESCRIPTION_EXP = 256
    ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_NAME_EXP = 256
    ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_DESCRIPTION_EXP = 256
    ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP = 128
    ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP = 128
    ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP = 128
    ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP = 128
    ZET_MAX_VALUE_INFO_CSTRING_EXP = 128
    ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP = 128
    ZEL_COMPONENT_STRING_SIZE = 64
    extend FFI::Library

    def self.ZE_MAKE_VERSION(major, minor)
      (major << 16 )|(minor & 0x0000ffff)
    end

    def self.ZE_BIT(i)
      1 << i
    end

    def self.ZE_MAKE_VERSION(major, minor)
      ( major << 16 )|( minor & 0x0000ffff)
    end

    VERSION_CURRENT = ZE_MAKE_VERSION(0, 91)

    def self.ZE_MAJOR_VERSION(ver = VERSION_CURRENT)
      ver >> 16
    end

    def self.ZE_MINOR_VERSION(ver = VERSION_CURRENT)
      ver & 0x0000ffff
    end

    module Handle
      def to_s
        s = '{ data: "'
        s << self[:data].to_a.collect { |v| "\\\\x%02x" % ((v + 256)%256) }.join
        s << '" }'
      end
    end

    module UUID
      def to_s
        a = self[:id].to_a
        s = "{ id: "
        s << "%02x" % a[15]
        s << "%02x" % a[14]
        s << "%02x" % a[13]
        s << "%02x" % a[12]
        s << "-"
        s << "%02x" % a[11]
        s << "%02x" % a[10]
        s << "-"
        s << "%02x" % a[9]
        s << "%02x" % a[8]
        s << "-"
        s << "%02x" % a[7]
        s << "%02x" % a[6]
        s << "-"
        s << "%02x" % a[5]
        s << "%02x" % a[4]
        s << "%02x" % a[3]
        s << "%02x" % a[2]
        s << "%02x" % a[1]
        s << "%02x" % a[0]
        s << " }"
      end
    end

    module KUUID
      def to_s
        a = self[:kid].to_a
        s = "{ kid: "
        s << "%02x" % a[15]
        s << "%02x" % a[14]
        s << "%02x" % a[13]
        s << "%02x" % a[12]
        s << "-"
        s << "%02x" % a[11]
        s << "%02x" % a[10]
        s << "-"
        s << "%02x" % a[9]
        s << "%02x" % a[8]
        s << "-"
        s << "%02x" % a[7]
        s << "%02x" % a[6]
        s << "-"
        s << "%02x" % a[5]
        s << "%02x" % a[4]
        s << "%02x" % a[3]
        s << "%02x" % a[2]
        s << "%02x" % a[1]
        s << "%02x" % a[0]
        a = self[:mid].to_a
        s << ", mid: "
        s << "%02x" % a[15]
        s << "%02x" % a[14]
        s << "%02x" % a[13]
        s << "%02x" % a[12]
        s << "-"
        s << "%02x" % a[11]
        s << "%02x" % a[10]
        s << "-"
        s << "%02x" % a[9]
        s << "%02x" % a[8]
        s << "-"
        s << "%02x" % a[7]
        s << "%02x" % a[6]
        s << "-"
        s << "%02x" % a[5]
        s << "%02x" % a[4]
        s << "%02x" % a[3]
        s << "%02x" % a[2]
        s << "%02x" % a[1]
        s << "%02x" % a[0]
        s << " }"
      end
    end

    module Version
      def to_s
        "\#{self[:major]}.\#{self[:minor]}"
      end

      def to_i
        ZE.ZE_MAKE_VERSION(self[:major], self[:minor])
      end
      alias to_int to_i
    end

EOF

def print_union(name, union)
  print_union_with_namespace(:ZE, name, union)
end

def print_struct(name, struct)
  prepends = []
  if to_class_name(name).match('UUID')
    prepends << if to_class_name(name).match('ZEKernelUUID')
                  'KUUID'
                else
                  'UUID'
                end
  elsif to_class_name(name).match(/Handle\z/)
    prepends << 'Handle'
  end

  if struct.to_ffi.first[0] == ':stype' && !$struct_type_reject.include?(name)
    initializer = <<EOF

    def initialize(*args)
      super(*args)
      if(args.length == 0)
EOF
    ename = to_ffi_name(name)
    ename = case ename
            when /\A:ze_/
              "ZE_STRUCTURE_TYPE_#{ename.to_s[4..-3]}"
            when /\A:zet_/
              "ZET_STRUCTURE_TYPE_#{ename.to_s[5..-3]}"
            when /\A:zes_/
              "ZES_STRUCTURE_TYPE_#{ename.to_s[5..-3]}"
            when /\A:zel_/
              "ZEL_STRUCTURE_TYPE_#{ename.to_s[5..-3]}"
            else
              raise "Unrecognized namespace for #{ename}"
            end.upcase
    ename = $struct_type_conversion_table[ename] if $struct_type_conversion_table[ename]

    initializer << <<EOF
        self[:stype] = :#{ename}
      end
    end
EOF
  else
    initializer = nil
  end

  print_struct_with_namespace(:ZE, name, struct, prepends: prepends, initializer: initializer, close: false)
end

$int_scalars.each do |k, v|
  next if to_ffi_name(k).match('_flags_t') && !to_ffi_name(k).match('_packed_')

  puts <<EOF
  typedef #{to_ffi_name(v)}, #{to_ffi_name(k)}

EOF
end

def is_primitive_type?(t_type)
  [YAMLCAst::Int, YAMLCAst::Void, YAMLCAst::Float, YAMLCAst::Char, YAMLCAst::Bool, YAMLCAst::Complex,
   YAMLCAst::Imaginary, YAMLCAst::Pointer].any? do |tt|
    t_type.is_a?(tt)
  end
end

# We need to sort the type, so they are defined in order
# We don't support cycle.

# We put Enum and $objecs first.
# If not, they are not added by the BFS, not sure why
all_type_sorted = $all_types.group_by do |t|
  if $objects.include?(t.name) || t.type.is_a?(YAMLCAst::Enum)
    :sorted
  else
    :to_be_sorted
  end
end

# Transform into set for fast `include`
all_type_sorted[:sorted] = all_type_sorted[:sorted].to_set

# Do the recursion, to put the child first. We mutate `all_type_sorted[:sorted]`
dfs = lambda do |node|
  return if is_primitive_type?(node) || node.is_a?(YAMLCAst::CustomType)

  case node.type
  when YAMLCAst::Struct
    members = STRUCT_MAP[node.name]
    members.each do |m|
      m_type = m.type
      m_type = m_type.type while m_type.is_a?(YAMLCAst::Array)
      dfs.call(m_type)
    end

  when YAMLCAst::Pointer
    if node.type.type.is_a?(YAMLCAst::Function)
      func = node.type.type
      types_to_check = [func.type]
      types_to_check = func.params.map(&:type) if func.params

      types_to_check.each do |ft|
        ft = ft.type while ft.is_a?(YAMLCAst::Pointer) || ft.is_a?(YAMLCAst::Array)
        dfs.call(ft)
      end
    end
  when YAMLCAst::Union
    # TODO
  when YAMLCAst::CustomType
    dfs.call(node.type)
  when YAMLCAst::Enum
    # Enums have no dependencies
  else
    raise
  end

  all_type_sorted[:sorted] << node
end

# Now sort all type who required it
all_type_sorted[:to_be_sorted].each do |t|
  dfs.call(t)
end

all_type_sorted[:sorted].each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    print_enum(t.name, enum)
  elsif $objects.include?(t.name)
    print_ze_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = $all_structs.find { |s| t.type.name == s.name }
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union(t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  end
end

puts <<~EOF
    class ZETypedValue
      def to_s
        case self[:type]
        when :ZET_VALUE_TYPE_UINT32
          self[:value][:ui32].to_s
        when :ZET_VALUE_TYPE_UINT64
          self[:value][:ui64].to_s
        when :ZET_VALUE_TYPE_FLOAT32
          self[:value][:fp32].to_s
        when :ZET_VALUE_TYPE_FLOAT64
          self[:value][:fp64].to_s
        when :ZET_VALUE_TYPE_BOOL8
          self[:value][:b8].to_s
        else
          self[:value][:ui64].to_s
        end
      end
    end
  end
EOF
