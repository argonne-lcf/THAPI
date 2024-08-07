require_relative 'gen_mpi_library_base'

def popcount(x)
  raise 'Unsigned integer needed!' if x < 0

  c = 0
  while x != 0
    c += 1 if x & 1 != 0
    x >>= 1
  end
  c
end

def firstbitpos(x)
  raise 'Signed positive integer needed!' if x <= 0

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
  enum.members.each do |m|
    if m.val
      counter = eval(m.val)
    else
      counter += 1
    end
    if counter > 0 && popcount(counter) == 1
      members.push [m.name.to_sym, firstbitpos(counter)]
    elsif counter == 0
      default = m.name.to_sym
    else
      r = [m.name.to_sym]
      r.push m.val || counter
      special_values.push r
    end
  end
  print_lambda = lambda { |m|
    "#{m[0].inspect}, #{m[1]}"
  }
  puts <<EOF
  #{to_class_name(name)} = mpibitmask #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]
EOF
  if default
    puts <<EOF
  #{to_class_name(name)}.default = #{default.inspect}
EOF
  end
  unless special_values.empty?
    puts <<EOF
  # #{special_values.collect(&print_lambda).join(",\n  # ")}
EOF
  end
  puts "\n"
end

def print_enum(name, enum)
  members = enum.members.collect do |m|
    r = [m.name.to_sym]
    r.push m.val if m.val
    r
  end
  print_lambda = lambda { |m|
    s = "#{m[0].inspect}"
    s << ", #{m[1]}" if m.size == 2
    s
  }
  puts <<EOF
  #{to_class_name(name)} = mpienum #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]

EOF
end

def print_mpi_object(object)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(object)}

EOF
end

puts <<~EOF
  require 'ffi'
  module FFI

    class MPIStruct < Struct
      def self.enclosing_module
        MPI
      end

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
    class MPIEnum < Enum
    end
    class MPIUnion < Union
      def self.enclosing_module
        MPI
      end
    end
    class MPIBitmask < Bitmask
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
      def mpibitmask(*args)
        generic_enum(FFI::MPIBitmask, *args)
      end
      def mpienum(*args)
        generic_enum(FFI::MPIEnum, *args)
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
  module MPI
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
EOF

def close_type(name)
  $all_types.select do |t|
    t.type.is_a?(YAMLCAst::CustomType) && t.type.name == name
  end.each do |t|
    puts <<EOF
  typedef #{to_ffi_name(name)}, #{to_ffi_name(t.name)}

EOF
    close_type(t.name)
  end
end

def print_union(name, union)
  members = union.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    s << if m[1].is_a?(Array)
           "[ #{m[1][0]}, #{m[1][1]} ]"
         else
           "#{m[1]}"
         end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::MPIUnion
    layout #{members.collect(&print_lambda).join(",\n" + (' ' * 11))}
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

def print_struct(name, struct)
  members = struct.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    s << if m[1].is_a?(Array)
           "[ #{m[1][0]}, #{m[1][1]} ]"
         else
           "#{m[1]}"
         end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::MPIStruct
EOF
  puts <<EOF if to_class_name(name).match('UUID')
    prepend UUID
EOF
  puts <<EOF if to_class_name(name).match(/Handle\z/)
    prepend Handle
EOF
  puts <<EOF
    layout #{members.collect(&print_lambda).join(",\n" + (' ' * 11))}
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
  close_type(name)
end

def print_function_pointer_type(name, func)
  type, params = func.to_ffi
  puts <<EOF
  callback #{to_ffi_name(name)},
           [ #{params.join(",\n" + (' ' * 13))} ],
           #{type}

EOF
end

def print_pointer_type(name)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(name)}

EOF
end

def print_int_type(name, t_name)
  puts <<EOF
  typedef #{to_ffi_name(t_name)}, #{to_ffi_name(name)}
EOF
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if false
      print_bitfield(t.name, enum)
    else
      print_enum(t.name, enum)
    end
  elsif $objects.include?(t.name)
    print_mpi_object(t.name)
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
  elsif t.type.is_a?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.is_a?(YAMLCAst::Int)
    print_int_type(t.name, t.type.name)
  end
end

puts <<~EOF
  end
EOF
