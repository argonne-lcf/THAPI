require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast'
require_relative '../utils/LTTng'
require_relative '../utils/meta_parameters'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "hipResult"

$hip_api_yaml = YAML::load_file("hip_api.yaml")
$hip_api = YAMLCAst.from_yaml_ast($hip_api_yaml)

hip_funcs_e = $hip_api["functions"]

hip_types_e = $hip_api["typedefs"]

all_types = hip_types_e
all_structs = $hip_api["structs"]

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat( types.filter_map { |t|
      t.name if t.type.kind_of?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    } ).uniq!
    break if sz == arr.size
    sz = arr.size
  end
  arr
end

def transitive_closure_map(types, map)
  sz = map.size
  loop do
    types.select { |t|
      t.type.kind_of?(YAMLCAst::CustomType) && map.include?(t.type.name)
    }.each { |t| map[t.name] = map[t.type.name] }
    break if sz == map.size
    sz = map.size
  end
end

def find_types(types, cast_type, arr = nil)
  res = types.select { |t| t.type.kind_of? cast_type }.collect { |t| t.name }
  if arr
    arr.concat res
    res = arr
  end
  transitive_closure(types, res)
end

def find_types_map(types, cast_type, map)
  res = types.select { |t| t.type.kind_of? cast_type }.each { |t|
    map[t.name] = map[t.type.name]
  }
  transitive_closure_map(types, res)
end

HIP_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
transitive_closure(all_types, HIP_OBJECTS)
HIP_OBJECTS.push "hipGraphicsResource_t"

HIP_INT_SCALARS = %w(size_t uintptr_t uint32_t uint64_t int short char)
HIP_INT_SCALARS.concat [ "long long", "unsigned long long", "unsigned long long int", "unsigned int", "unsigned short", "unsigned char" ]
find_types(all_types, YAMLCAst::Int, HIP_INT_SCALARS)

HIP_ENUM_SCALARS = find_types(all_types, YAMLCAst::Enum)
HIP_STRUCT_TYPES = find_types(all_types, YAMLCAst::Struct)
HIP_UNION_TYPES = find_types(all_types, YAMLCAst::Union)
HIP_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

HIP_POINTER_TYPES.delete("hipGraphicsResource_t")

HIP_STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    HIP_STRUCT_MAP[t.name] = t.type.members
  else
    mapped = all_structs.find { |str| str.name == t.type.name }
    HIP_STRUCT_MAP[t.name] = mapped.members if mapped
  end
}
transitive_closure_map(all_types, HIP_STRUCT_MAP)

INIT_FUNCTIONS = /.*/

FFI_TYPE_MAP =  {
  "unsigned char" => "ffi_type_uint8",
  "char" => "ffi_type_sint8",
  "unsigned short" => "ffi_type_uint16",
  "short" => "ffi_type_sint16",
  "unsigned int" => "ffi_type_uint32",
  "int" => "ffi_type_sint32",
  "unsigned long long" => "ffi_type_uint64",
  "unsigned long long int" => "ffi_type_uint64",
  "long long" => "ffi_type_sint64",
  "uint32_t" => "ffi_type_uint32",
  "uint64_t" => "ffi_type_uint64",
  "float" => "ffi_type_float",
  "double" => "ffi_type_double",
  "size_t" => "ffi_type_pointer",
}
transitive_closure_map(all_types, FFI_TYPE_MAP)

HIP_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

HIP_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

HIP_FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

module YAMLCAst
  class Declaration
    def lttng_type
      r = type.lttng_type
      r.name = name
      case type
      when Struct, Union
        r.expression = "&#{name}"
      when CustomType
        case type.name
        when *HIP_STRUCT_TYPES, *HIP_UNION_TYPES
          r.expression = "&#{name}"
        else
          r.expression = name
        end
      else
        r.expression = name
      end
      r
    end
  end

  class Type
    def lttng_type
      raise "Unsupported type #{self}!"
    end
  end

  class Void
    def lttng_type
      nil
    end
  end

  class Int
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Float
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_float
      ev.type = name
      ev
    end
  end

  class Char
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Bool
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Struct
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(struct #{name})"
      ev
    end

    def [](name)
      members.find { |m| m.name == name }
    end
  end

  class Union
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(union #{name})"
      ev
    end
  end

  class Enum
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = "enum #{name}"
      ev
    end
  end

  class CustomType
    def lttng_type
      ev = LTTng::TracepointField::new
      case name
      when *HIP_OBJECTS, *HIP_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when *HIP_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *HIP_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *HIP_STRUCT_TYPES, *HIP_UNION_TYPES
        ev.macro = :ctf_array_text
        ev.type = :uint8_t
        ev.length = "sizeof(#{name})"
      else
        super
      end
      ev
    end
  end

  class Pointer
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer_hex
      ev.type = :uintptr_t
      ev.cast = "uintptr_t"
      ev
    end
  end

  class Array
    def lttng_type(length: nil, length_type: nil)
      ev = LTTng::TracepointField::new
      if length
        ev.length = length
      elsif self.length
        ev.length = self.length
      else
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
        return ev
      end
      if length_type
        lttng_arr_type = "sequence"
        ev.length_type = length_type
      else
        lttng_arr_type = "array"
      end
      case type
      when YAMLCAst::Pointer
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = :uintptr_t
      when YAMLCAst::Int
        ev.macro = :"ctf_#{lttng_arr_type}"
        ev.type = type.name
      when YAMLCAst::Float
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = HIP_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *HIP_OBJECTS, *HIP_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *HIP_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *HIP_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *HIP_STRUCT_TYPES, *HIP_UNION_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(#{type.name})"
          end
        when "uint8_t"
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(uint8_t)"
          end
        else
          super
        end
      else
        super
      end
      ev
    end
  end
end

class Member
  def initialize(command, member, prefix, dir = :start)
    @member = member
    @dir = dir
    @prefix = prefix
    name = "#{prefix}#{MEMBER_SEPARATOR}#{member.name}"
    expr = "#{prefix} ? #{prefix}->#{member.name} : 0"
    @lttng_type = member.type.lttng_type
    @lttng_type.name = name
    @lttng_type.expr = expr
  end

   def lttng_in_type
     @dir == :start ? @lttng_type : nil
   end

   def lttng_out_type
     @dir == :start ? nil : @lttng_type
   end
end

class Command
  attr_reader :tracepoint_parameters
  attr_reader :meta_parameters
  attr_reader :prologues
  attr_reader :epilogues
  attr_reader :function

  def initialize(function)
    @function = function
    @tracepoint_parameters = []
    @meta_parameters = AUTO_META_PARAMETERS.collect { |klass| klass.create_if_match(self) }.compact
    @meta_parameters += META_PARAMETERS[@function.name].collect { |type, args|
      type::new(self, *args)
    }
    @prologues = PROLOGUES[@function.name]
    @epilogues = EPILOGUES[@function.name]
  end

  def name
    @function.name
  end

  def decl_pointer(name = pointer_name)
    YAMLCAst::Declaration::new(name: name, type: YAMLCAst::Pointer::new(type: @function.type), storage: "typedef").to_s
  end

  def decl
    @function.to_s
  end

  def pointer_name
    name + "_ptr"
  end

  def pointer_type_name
    name + "_t"
  end

  def type
    @function.type.type
  end

  def parameters
    @function.type.params
  end

  def init?
    name.match(INIT_FUNCTIONS)
  end

  def has_return_type?
    if type && !type.kind_of?(YAMLCAst::Void)
      true
    else
      false
    end
  end

  def [](name)
    # special case when querying the return value
    if name == :result
      return YAMLCAst::Declaration.new(name: "#{RESULT_NAME}", type: type)
    end
    path = name.split(/->/)
    if path.length == 1
      res = parameters.find { |p| p.name == name }
      return res if res
      @tracepoint_parameters.find { |p| p.name == name }
    else
      param_name = path.shift
      res = parameters.find { |p| p.name == param_name }
      return nil unless res
      path.each { |n|
        res = HIP_STRUCT_MAP[res.type.type.name].find { |m| m.name == n }
        return nil unless res
      }
      return res
    end
  end

end

def register_meta_parameter(method, type, *args)
  META_PARAMETERS[method].push [type, args]
end

def register_meta_struct(method, name, type)
  raise "Unknown struct: #{type}!" unless HIP_STRUCT_TYPES.include?(type)
  HIP_STRUCT_MAP[type].each { |m|
    META_PARAMETERS[method].push [Member, [m, name]]
  }
end

def register_prologue(method, code)
  PROLOGUES[method].push(code)
end

def register_epilogue(method, code)
  EPILOGUES[method].push(code)
end

AUTO_META_PARAMETERS = []
META_PARAMETERS = Hash::new { |h, k| h[k] = [] }
PROLOGUES = Hash::new { |h, k| h[k] = [] }
EPILOGUES = Hash::new { |h, k| h[k] = [] }

$hip_meta_parameters = YAML::load_file(File.join(SRC_DIR,"hip_meta_parameters.yaml"))
$hip_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$hip_commands = hip_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

HIP_POINTER_NAMES = $hip_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h
