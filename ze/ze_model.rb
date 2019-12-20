require 'yaml'
require 'pp'
require './yaml_ast'

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

provider = :lttng_ust_ze

ze_api = YAMLCAst.from_yaml_ast(YAML::load_file("ze_api.yaml"))
zex_api = YAMLCAst.from_yaml_ast(YAML::load_file("zex_api.yaml"))
zet_api = YAMLCAst.from_yaml_ast(YAML::load_file("zet_api.yaml"))

ze_funcs_e = ze_api["functions"]
zex_funcs_e = zex_api["functions"]
zet_funcs_e = zet_api["functions"]

ze_types_e = ze_api["typedefs"]

ZE_OBJECTS = ze_types_e.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
ZE_INT_SCALARS = %w(intptr_t size_t int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t ze_bool_t)
ZE_ENUM_SCALARS = ze_types_e.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
ZE_STRUCT_TYPES = ze_types_e.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
ZE_UNION_TYPES = ze_types_e.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
ZE_POINTER_TYPES = ze_types_e.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

ZE_STRUCT_MAP = {}
ze_types_e.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    ZE_STRUCT_MAP[t.name] = t.type.members
  else
    ZE_STRUCT_MAP[t.name] = ze_api["structs"].find { |str| str.name == t.type.name }.members
  end
}

INIT_FUNCTIONS = /zeInit/

FFI_TYPE_MAP =  {
 "uint8_t" => "ffi_type_uint8",
 "int8_t" => "ffi_type_sint8",
 "uint16_t" => "ffi_type_uint16",
 "int16_t" => "ffi_type_sint16",
 "uint32_t" => "ffi_type_uint32",
 "int32_t" => "ffi_type_sint32",
 "uint64_t" => "ffi_type_uint64",
 "int64_t" => "ffi_type_sint64",
 "float" => "ffi_type_float",
 "double" => "ffi_type_double",
 "intptr_t" => "ffi_type_pointer",
 "size_t" => "ffi_type_pointer",
 "ze_bool_t" => "ffi_type_uint8"
}

ZE_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ZE_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}



ZE_FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

module LTTNG
  class TracepointEvent
    attr_accessor :macro
    attr_accessor :name
    attr_accessor :expression
    attr_accessor :type
    attr_accessor :provider_name
    attr_accessor :enum_name
    attr_accessor :length
    attr_accessor :length_type

    def call_string
      str = "#{macro}("
      str << [ @provider_name, @enum_name, @type, @name, @expression, @length_type, @length ].compact.join(", ")
      str << ")"
    end
  end
end

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
        when *ZE_STRUCT_TYPES, *ZE_UNION_TYPES
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

  class Int
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Float
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_float
      ev.type = name
      ev
    end
  end

  class Char
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Bool
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Struct
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(#{name})"
      ev
    end
  end

  class Union
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(#{name})"
      ev
    end
  end

  class Enum
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_integer
      ev.type = "enum #{name}"
      ev
    end
  end

  class CustomType
    def lttng_type
      ev = LTTNG::TracepointEvent::new
      case name
      when *ZE_OBJECTS, *ZE_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :intptr_t
      when *ZE_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *ZE_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *ZE_STRUCT_TYPES, *ZE_UNION_TYPES
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
      ev = LTTNG::TracepointEvent::new
      ev.macro = :ctf_integer_hex
      ev.type = :intptr_t
      ev
    end
  end

  class Array
    def lttng_type(length: nil, length_type: nil)
      ev = LTTNG::TracepointEvent::new
      if length
        ev.length = length
      elsif self.length
        ev.length = self.length
      else
        ev.macro = :ctf_integer_hex
        ev.type = :intptr_t
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
        ev.type = :intptr_t
      when YAMLCAst::Int
        ev.macro = :"ctf_#{lttng_arr_type}"
        ev.type = type.name
      when YAMLCAst::Float
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = ZE_FLOAT_SCALARS_MAP[type.type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *ZE_OBJECTS, *ZE_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :intptr_t
        when *ZE_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *ZE_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *ZE_STRUCT_TYPES, *ZE_UNION_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(#{type.name})"
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
    name = "#{prefix}_#{member.name}"
    expr = "#{prefix} != NULL ? #{prefix}->#{member.name} : 0"
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

  def initialize(function)
    @function = function
    @tracepoint_parameters = []
    @meta_parameters = AUTO_META_PARAMETERS.collect { |klass| klass.create_if_match(self) }.compact
    @meta_parameters += META_PARAMETERS[@function.name].collect { |type, args|
      type::new(self, *args)
    }
    @init      = @function.name.match(INIT_FUNCTIONS)
    @prologues = PROLOGUES[@function.name]
    @epilogues = EPILOGUES[@function.name]
  end

  def name
    @function.name
  end

  def type
    @function.type.type
  end

  def parameters
    @function.type.params
  end

  def has_return_type?
    if type && !type.kind_of?(YAMLCAst::Void)
      true
    else
      false
    end
  end

  def [](name)
    res = parameters.find { |p| p.name == name }
    return res if res
    @tracepoint_parameters.find { |p| p.name == name }
  end

end

def register_meta_parameter(method, type, *args)
  META_PARAMETERS[method].push [type, args]
end

def register_meta_struct(method, name, type)
  raise "Unknown struct: #{type}!" unless ZE_STRUCT_TYPES.include?(type)
  ZE_STRUCT_MAP[type].each { |m|
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

$ze_commands = ze_funcs_e.collect { |func|
  Command::new(func)
}


