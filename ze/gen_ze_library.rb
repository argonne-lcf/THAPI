require_relative 'ze_model'
require_relative 'gen_probe_base.rb'


bitfields = {}
enums = {}
structs = {}
unions = {}
objects = []
int_scalars = ZE_INT_SCALARS
float_scalars = [ "double", "float" ]
events = {}


res = {
  "enums" => enums,
  "bitfields" => bitfields,
  "structs" => structs,
  "unions" => unions,
  "objects" => objects,
  "int_scalars" => int_scalars,
  "float_scalars" => float_scalars,
  "events" => events
}

all_types = $ze_api["typedefs"]# + $zet_api["typedefs"]
all_structs = $ze_api["structs"]# + $zet_api["structs"]
all_enums = $ze_api["enums"]# + $zet_api["enums"]
$all_struct_names = []

objects = all_types.select { |t|
  t.type.kind_of?(YAMLCAst::Pointer) &&
  t.type.type.kind_of?(YAMLCAst::Struct)
}.collect { |t| t.name }

all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    objects.push t.name
  end
}

def ZE_BIT(i)
  1 << i
end

def ZE_MAKE_VERSION(major, minor)
  ( major << 16 )|( minor & 0x0000ffff )
end

def ZE_MAJOR_VERSION(ver)
  ver >> 16
end

def ZE_MINOR_VERSION(ver)
  ver & 0x0000ffff
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
  puts "\n"
end

def to_class_name(name)
  n = name.gsub(/_t\z/, "").gsub(/\Aze_/, "").split("_").collect(&:capitalize).join
  n.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("Ipc", "IPC").gsub("Api","API").gsub("P2p", "P2P")
end

def to_ffi_name(name)
  name.to_sym.inspect
end

def print_enum(name, enum)
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
  #{to_class_name(name)} = enum #{to_ffi_name(name)},
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

  def self.ZE_MAJOR_VERSION(ver)
    ver >> 16
  end

  def self.ZE_MINOR_VERSION(ver)
    ver & 0x0000ffff
  end

EOF

module YAMLCAst
  class Struct
    def to_ffi
      res = []
      members.each { |m|
        mt = case m.type
        when Array
          m.type.to_ffi
        when Pointer
          ":pointer"
        else
          to_ffi_name(m.type.name)
        end
        res.push [to_ffi_name(m.name), mt]
      }
      res
    end
  end

  class Array
    def to_ffi
      t = case type
      when Pointer
        ":pointer"
      else
       to_ffi_name(type.name)
      end
      [ t, length ]
    end
  end

  class Function
    def to_ffi
      t = to_ffi_name(type.name)
      p = params.collect { |par|
        if par.type.kind_of?(Pointer)
          if par.type.type.respond_to?(:name) &&
             $all_struct_names.include?(par.type.type.name)
            "#{to_class_name(par.type.type.name)}.ptr"
          else
            ":pointer"
          end
        else
          to_ffi_name(par.type.name)
        end
      }
      [t, p]
    end
  end
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
  class #{to_class_name(name)} < FFI::Struct
    layout #{members.collect(&print_lambda).join(",\n"+" "*11)}
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

ze_bool = all_types.find { |t| t.name == "ze_bool_t" }

puts <<EOF
  typedef #{to_ffi_name(ze_bool.type.name)}, #{to_ffi_name(ze_bool.name)}

EOF

CL_OBJECTS.each { |o|
  print_ze_object(o)
}

all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = all_enums.find { |e| t.type.name == e.name }
    if enum.members.find { |m| m.val && m.val.match("ZE_BIT") }
      print_bitfield(t.name, enum)
    else
      print_enum(t.name, enum)
    end
  elsif objects.include?(t.name)
    print_ze_object(t.name)
  elsif t.type.kind_of? YAMLCAst::Struct
    struct = all_structs.find { |s| t.type.name == s.name }
    next unless struct
    print_struct(t.name, struct)
    $all_struct_names.push(t.name)
  elsif t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Function)
    print_function_pointer_type(t.name, t.type.type)
  end
}

puts <<EOF
end
EOF
