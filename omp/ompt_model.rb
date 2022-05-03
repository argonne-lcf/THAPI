require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast'
require_relative '../utils/LTTng'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

START = "entry"
STOP = "exit"
SUFFIXES = { :start => START, :stop => STOP }

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

provider = :lttng_ust_ompt

$ompt_api_yaml = YAML::load_file("ompt_api.yaml")
$ompt_api = YAMLCAst.from_yaml_ast($ompt_api_yaml)

ompt_funcs_e = $ompt_api["functions"]
ompt_types_e = $ompt_api["typedefs"]

all_types = ompt_types_e
all_structs = $ompt_api["structs"]
OMPT_OBJECTS = ["ompt_device_t", "ompt_buffer_t","ompd_address_space_handle_t", "ompd_thread_handle_t", "ompd_parallel_handle_t",
                "ompd_task_handle_t","ompd_address_space_context_t", "ompd_thread_context_t"]
OMPT_INT_SCALARS = %w(int size_t uint8_t int64_t int32_t uint64_t char) + ["unsigned int"]
OMPT_FLOAT_SCALARS = ["double"]
OMPT_SCALARS = OMPT_INT_SCALARS + OMPT_FLOAT_SCALARS

all_types.each { |t|
  if  ( t.type.kind_of?(YAMLCAst::CustomType) || t.type.kind_of?(YAMLCAst::Int) ) && OMPT_INT_SCALARS.include?(t.type.name)
    OMPT_INT_SCALARS.push t.name
  end
}


OMPT_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
OMPT_STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
OMPT_UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
OMPT_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
OMPT_CALLBACKS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Function) && t.name.match(/ompt_callback_.*_t/) }
                          .reject { |t| ["ompt_callback_buffer_complete_t","ompt_callback_buffer_request_t"].include?(t.name) }
                          .collect { |t| YAMLCAst::Declaration.new(name: t.name.gsub(/_t\z/,"")+"_func", type: t.type.type)}

OMPT_STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of?(YAMLCAst::Struct) && !OMPT_OBJECTS.include?(t.name) }.each { |t|
  if t.type.members
    OMPT_STRUCT_MAP[t.name] = t.type.members
  else
    OMPT_STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}

FFI_TYPE_MAP =  {
 "uint8_t" => "ffi_type_uint8",
 "uint64_t" => "ffi_type_uint64",
 "int64_t" => "ffi_type_sint64",
 "double" => "ffi_type_double",
 "uintptr_t" => "ffi_type_pointer",
 "size_t" => "ffi_type_pointer",
 "unsigned int" => "ffi_type_uint",
 "int" => "ffi_type_int",
 "char" => "ffi_type_char"
}

OMPT_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

OMPT_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

OMPT_FLOAT_SCALARS_MAP = {"double" => "uint64_t"}

INIT_FUNCTIONS=/None/

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
        when *OMPT_STRUCT_TYPES, *OMPT_UNION_TYPES
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
      when *OMPT_OBJECTS, *OMPT_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when *OMPT_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *OMPT_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *OMPT_STRUCT_TYPES, *OMPT_UNION_TYPES
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
        ev.type = OMPT_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *OMPT_OBJECTS, *OMPT_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *OMPT_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *OMPT_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *OMPT_STRUCT_TYPES, *OMPT_UNION_TYPES
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
        res = OMPT_STRUCT_MAP[res.type.type.name].find { |m| m.name == n }
        return nil unless res
      }
      return res
    end
  end

end

class MetaParameter
  attr_reader :name
  attr_reader :command
  def initialize(command, name)
    @command = command
    @name = name
  end

  def lttng_type
    @lttng_type
  end

  def check_for_null(expr, incl = true)
    list = expr.split("->")
    if list.length == 1
      if incl
        return [expr]
      else
        return []
      end
    else
      res = []
      pre = ""
      list[0..(incl ? -1 : -2)].each { |n|
        pre += n
        res.push(pre)
        pre += "->"
      }
      return res
    end
  end

  def sanitize_expression(expr, checks = check_for_null(expr, false), default = 0)
    if checks.empty?
      expr
    else
      "(#{checks.join(" && ")} ? #{expr} : #{default})"
    end
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
      checks = check_for_null("#{size}") + check_for_null("#{name}")
      sz = sanitize_expression("*#{size}", checks)
      st = "#{s.type.type}"
    else
      checks = check_for_null("#{name}")
      sz = sanitize_expression("#{size}", checks)
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
    lttngt.expression = sanitize_expression("#{name}")
    @lttng_type = lttngt
  end
end

def register_meta_parameter(method, type, *args)
  META_PARAMETERS[method].push [type, args]
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

$ompt_meta_parameters = YAML::load_file(File.join(SRC_DIR, "ompt_meta_parameters.yaml"))
$ompt_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$stderr.puts META_PARAMETERS

$ompt_commands = OMPT_CALLBACKS.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

OMPT_POINTER_NAMES = $ompt_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h


