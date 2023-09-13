require_relative 'gen_omp_library_base'

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
  #{to_class_name(name)} = ompbitmask #{to_ffi_name(name)},
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

def print_enum(name, enum)
  return print_version_enum_struct(name) if name.match(/_version_t\z/)
  members = enum.members.filter { |m| m.name != "ompt_callback_master"}.collect { |m|
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
  #{to_class_name(name)} = ompenum #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]

EOF
end


puts <<EOF
require 'ffi'
module FFI

  class OMPStruct < Struct
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

  class OMPEnum < Enum
  end

  class OMPUnion < Union
  end

  class OMPBitmask < Bitmask
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
    def ompbitmask(*args)
      generic_enum(FFI::OMPBitmask, *args)
    end
    def ompenum(*args)
      generic_enum(FFI::OMPEnum, *args)
    end
  end

  class Pointer
    def to_s
      "0x%016x" % address
    end
  end
end

module OMP
  extend FFI::Library

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
  class #{to_class_name(name)} < FFI::OMPUnion
    layout #{members.collect(&print_lambda).join(",\n"+" "*11)}
EOF
  if name == "ompt_data_t"
    puts <<EOF
    def to_s
      "0x%016x" % self[:value]
    end
EOF
  end
  puts <<EOF
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
  class #{to_class_name(name)} < FFI::OMPStruct
EOF
  puts <<EOF
    layout #{members.collect(&print_lambda).join(",\n"+" "*11)}
EOF
  puts <<EOF
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

$all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if enum.name.end_with?("flag_t") 
      print_bitfield(t.name, enum)
    else
      print_enum(t.name, enum)
    end
  elsif t.type.kind_of? YAMLCAst::Struct
    struct = $all_structs.find { |s| t.type.name == s.name }
    next unless struct
    print_struct(t.name, struct)
  elsif t.type.kind_of? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union
    print_union(t.name, union)
  end
}

puts <<EOF
end
EOF
