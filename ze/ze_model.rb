require 'yaml'
require 'pp'
require './yaml_ast'

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

provider = :lttng_ust_ze

ze_api = YAMLCAst.from_yaml_ast(YAML::load_file("ze_api.yaml"))
zet_api = YAMLCAst.from_yaml_ast(YAML::load_file("zet_api.yaml"))

ze_funcs_e = ze_api["functions"]
zet_funcs_e = zet_api["functions"]

ze_types_e = ze_api["typedefs"]
zet_types_e = zet_api["typedefs"]

all_types = ze_types_e + zet_types_e
all_structs = ze_api["structs"] + zet_api["structs"]

CL_OBJECTS = %w(cl_platform_id cl_device_id cl_context cl_command_queue cl_mem cl_program cl_kernel cl_event cl_sampler)
ZE_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    ZE_OBJECTS.push t.name
  end
}
ZE_INT_SCALARS = %w(intptr_t size_t int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t ze_bool_t)
ZE_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
ZE_STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name } + [ "zet_core_callbacks_t" ]
ZE_UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
ZE_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

ZE_STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    ZE_STRUCT_MAP[t.name] = t.type.members
  else
    ZE_STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
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
    attr_accessor :cast

    def call_string
      str = "#{macro}("
      str << [ @provider_name, @enum_name, @type, @name, @cast ? "(#{@cast})(#{@expression})" : @expression, @length_type, @length ].compact.join(", ")
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
      when *ZE_OBJECTS, *ZE_POINTER_TYPES, *CL_OBJECTS
        ev.macro = :ctf_integer_hex
        ev.type = :intptr_t
        ev.cast = "intptr_t"
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
      ev.cast = "intptr_t"
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
        ev.cast = "intptr_t"
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
        ev.type = ZE_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *ZE_OBJECTS, *ZE_POINTER_TYPES, *CL_OBJECTS
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
  attr_reader :function

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
    res = parameters.find { |p| p.name == name }
    return res if res
    @tracepoint_parameters.find { |p| p.name == name }
  end

end

module In
  def lttng_in_type
    @lttng_in_type
  end
end

module Out
  def lttng_out_type
    @lttng_out_type
  end
end

class MetaParameter
  attr_reader :name
  attr_reader :command
  def initialize(command, name)
    @command = command
    @name = name
  end

  def lttng_in_type
    nil
  end

  def lttng_out_type
    nil
  end
end

class InString < MetaParameter
  prepend In
  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTNG::TracepointEvent::new
    ev.macro = :ctf_string
    ev.name = "#{name}_val"
    ev.expression = "#{name}"
    @lttng_in_type = ev
  end
end

class ScalarMetaParameter < MetaParameter
  attr_reader :type

  def initialize(command, name, type = nil)
    super(command, name)
    @type = type
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    if type
      st = eval(type)
    else
      st = t.type
    end
    lttngt = st.lttng_type
    lttngt.name = name + "_val"
    if lttngt.macro == :ctf_array_text
      lttngt.macro = :ctf_sequence_text
      lttngt.expression = "#{name}"
      lttngt.length = "(#{name} == NULL ? 0 : #{lttngt.length})"
      lttngt.length_type = "size_t"
    elsif type
      lttngt.expression = "(#{name} == NULL ? 0 : *(#{YAMLCAst::Pointer::new(type: st)})#{name})"
    else
      lttngt.expression = "(#{name} == NULL ? 0 : *#{name})"
    end
    @lttng_type = @lttng_type = lttngt
  end
end

class InOutScalar < ScalarMetaParameter
  prepend In
  prepend Out
  def initialize(command, name, type = nil)
    super
    @lttng_out_type = @lttng_in_type = @lttng_type
  end
end

class OutScalar < ScalarMetaParameter
  prepend Out
  def initialize(command, name, type = nil)
    super
    @lttng_out_type = @lttng_type
  end
end

class InScalar < ScalarMetaParameter
  prepend In
  def initialize(command, name, type = nil)
    super
    @lttng_in_type = @lttng_type
  end
end

class ArrayMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s
    if s.type.kind_of?(YAMLCAst::Pointer)
      sz = "(#{size} == NULL ? 0 : #{name} == NULL ? 0 : *#{size})"
      st = "#{s.type.type}"
    else
      sz = "(#{name} == NULL ? 0 : #{size})"
      st = "#{s.type}"
    end
    if t.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: sz, length_type: st)
    lttngt.name = name + "_vals"
    lttngt.expression = "#{name}"
    @lttng_type = lttngt
  end
end

class OutArray < ArrayMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
  end
end

class InArray < ArrayMetaParameter
  prepend In
  def initialize(command, name, size)
    super
    @lttng_in_type = @lttng_type
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

$ze_meta_parameters = YAML::load_file("ze_meta_parameters.yaml")
$ze_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$zet_meta_parameters = YAML::load_file("zet_meta_parameters.yaml")
$zet_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$ze_commands = ze_funcs_e.collect { |func|
  Command::new(func)
}

$zet_commands = zet_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ZE_POINTER_NAMES = ($ze_commands + $zet_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h

