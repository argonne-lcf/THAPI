require_relative 'gen_ze_library_base.rb'

def ZE_BIT(i)
  1 << i
end

def popcount(x)
  raise "Unsigned integer needed!" if x < 0
  c = 0
  while x != 0
    c += 1 if x & 1 != 0
    x >>= 1
  end
  c
end

def firstbitpos(x)
  raise "Signed positive integer needed!" if x <= 0
  r = 0
  while x & 1 == 0
    r += 1
    x >>= 1
  end
  r
end

def print_bitfield(name, enum)
  special_values = []
  members = []
  default = nil
  counter = 0
  enum.members.each { |m|
    if m.val
      counter = eval(m.val)
    else
      counter += 1
    end
    if counter > 0 && popcount(counter) == 1
      members.push [ m.name.to_sym, firstbitpos(counter) ]
    else
      if counter == 0
        default = m.name.to_sym
      else
        r = [ m.name.to_sym ]
        if m.val
          r.push m.val
        else
          r.push counter
        end
        special_values.push r
      end
    end
  }
  print_lambda = lambda { |m|
    "#{m[0].inspect}, #{m[1]}"
  }
  puts <<EOF
  #{to_class_name(name)} = zebitmask #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]
EOF
  if default
    puts <<EOF
  #{to_class_name(name)}.default = #{default.inspect}
EOF
  end
  if !special_values.empty?
    puts <<EOF
  # #{special_values.collect(&print_lambda).join(",\n  # ")}
EOF
  end
  if to_ffi_name(name).match("_flag_t")
    puts "  #{to_class_name(name)}s = #{to_class_name(name)}"
    puts "  typedef #{to_ffi_name(name)}, #{to_ffi_name(name).gsub("_flag_t", "_flags_t")}"
  end
  puts "\n"
end

def print_version_enum_struct(name)
  puts <<EOF
  class #{to_class_name(name)} < FFI::ZEStruct
    prepend Version
    layout :minor, :uint16_t,
           :major, :uint16_t

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
  members = enum.members.collect { |m|
    r = [ m.name.to_sym ]
    r.push m.val if m.val
    r
  }
  print_lambda = lambda { |m|
    s = "#{m[0].inspect}"
    s << ", #{m[1]}" if m.size == 2
    s
  }
  puts <<EOF
  #{to_class_name(name)} = zeenum #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]

EOF
end

def print_ze_object(object)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(object)}

EOF
end

puts <<EOF
require 'ffi'
module FFI

  class ZEStruct < Struct
    def to_h
      members.zip(values).to_h
    end

    def to_s
      s = "{ "
      s << to_h.each.collect { |k, v|
        if v.kind_of? Array
          "\#{k}: [ \#{v.join(", ")} ]"
        else
          "\#{k}: \#{v}"
        end
      }.join(", ")
      s << " }"
      s
    end
  end
  class ZEEnum < Enum
  end
  class ZEUnion < Union
  end
  class ZEBitmask < Bitmask
    def default=(default)
      @default = default
    end

    def []
      if @default && query.size == 1
        if query[0] == 0
          return @default
        elsif query[0] == @default
          return 0
        end
      end
      super
    end

    def to_native(query, ctx)
      return 0 if query.nil?
      flat_query = [query].flatten
      return 0 if flat_query.size == 1 && query[0] == @default
      super
    end

    def from_native(val, ctx)
      return [@default] if val == 0 && @default
      super
    end
  end
  module Library
    def zebitmask(*args)
      generic_enum(FFI::ZEBitmask, *args)
    end
    def zeenum(*args)
      generic_enum(FFI::ZEEnum, *args)
    end
  end

  class Struct::InlineArray
    def to_s
      s = "[ "
      s << to_a.join(", ")
      s << " ]"
    end
  end

  class Pointer
    def to_s
      "0x%016x" % address
    end
  end
end
module ZE
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
  members = union.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    if m[1].kind_of?(Array)
      s << "[ #{m[1][0]}, #{m[1][1]} ]"
    else
      s << "#{m[1]}"
    end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::ZEUnion
    layout #{members.collect(&print_lambda).join(",\n"+" "*11)}
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

def print_struct(name, struct)
  members = struct.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    if m[1].kind_of?(Array)
      s << "[ #{m[1][0]}, #{m[1][1]} ]"
    else
      s << "#{m[1]}"
    end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::ZEStruct
EOF
  if to_class_name(name).match("UUID")
    if to_class_name(name).match("ZEKernelUUID")
  puts <<EOF
    prepend KUUID
EOF
    else
  puts <<EOF
    prepend UUID
EOF
    end
  elsif to_class_name(name).match(/Handle\z/)
puts <<EOF
    prepend Handle
EOF
  end
  puts <<EOF
    layout #{members.collect(&print_lambda).join(",\n"+" "*11)}
EOF
  if members.first[0] == ":stype" && !$struct_type_reject.include?(name)
    puts <<EOF

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
    puts <<EOF
        self[:stype] = :#{ename}
      end
    end
EOF
  end
  puts <<EOF
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

def print_function_pointer_type(name, func)
  type, params = func.to_ffi
  puts <<EOF
  callback #{to_ffi_name(name)},
           [ #{params.join(",\n"+" "*13)} ],
           #{type}

EOF
end

$int_scalars.each { |k, v|
  next if to_ffi_name(k).match("_flags_t")
  puts <<EOF
  typedef #{to_ffi_name(v)}, #{to_ffi_name(k)}

EOF
}

$all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if enum.members.find { |m| m.val && m.val.match("ZE_BIT") }
      print_bitfield(t.name, enum)
    else
      print_enum(t.name, enum)
    end
  elsif $objects.include?(t.name)
    print_ze_object(t.name)
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
  end
}

puts <<EOF
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
