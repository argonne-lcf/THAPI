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

provider = :lttng_ust_cuda

$cuda_api_yaml = YAML::load_file("cuda_api.yaml")
$cuda_exports_api_yaml = YAML::load_file("cuda_exports_api.yaml")

$cuda_api = YAMLCAst.from_yaml_ast($cuda_api_yaml)
$cuda_exports_api = YAMLCAst.from_yaml_ast($cuda_exports_api_yaml)

cuda_funcs_e = $cuda_api["functions"]
cuda_exports_funcs_e = $cuda_exports_api["functions"]

cuda_types_e = $cuda_api["typedefs"]
cuda_exports_type_e = $cuda_exports_api["typedefs"]

all_types = cuda_types_e + cuda_exports_type_e
all_structs = $cuda_api["structs"] + $cuda_exports_api["structs"]

CUDA_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && CUDA_OBJECTS.include?(t.type.name)
    CUDA_OBJECTS.push t.name
  end
}

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat( types.filter_map { |t|
      t.name if t.type.kind_of?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    } ).uniq!
    break if sz == arr.size
    sz = arr.size
  end
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

CUDA_INT_SCALARS = %w(size_t uint32_t cuuint32_t uint64_t cuuint64_t int short char
                      CUdevice CUdevice_v1
                      CUdeviceptr CUdeviceptr_v1 CUdeviceptr_v2
                      CUtexObject CUtexObject_v1 CUsurfObject CUsurfObject_v1
                      CUmemGenericAllocationHandle
                      VdpDevice VdpFuncId VdpVideoSurface VdpOutputSurface VdpStatus)
CUDA_INT_SCALARS.concat [ "long long", "unsigned long long", "unsigned long long int", "unsigned int", "unsigned short", "unsigned char" ]
CUDA_FLOAT_SCALARS = %w(float double)
CUDA_SCALARS = CUDA_INT_SCALARS + CUDA_FLOAT_SCALARS
CUDA_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
transitive_closure(all_types, CUDA_ENUM_SCALARS)
CUDA_STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
transitive_closure(all_types, CUDA_STRUCT_TYPES)
CUDA_UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
transitive_closure(all_types, CUDA_UNION_TYPES)
CUDA_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

CUDA_STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    CUDA_STRUCT_MAP[t.name] = t.type.members
  else
    CUDA_STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}
transitive_closure_map(all_types, CUDA_STRUCT_MAP)

INIT_FUNCTIONS = /cuInit|cuDriverGetVersion|cuGetExportTable|cuDeviceGetCount/

FFI_TYPE_MAP =  {
 "unsigned char" => "ffi_type_uint8",
 "char" => "ffi_type_sint8",
 "unsigned short" => "ffi_type_uint16",
 "short" => "ffi_type_sint16",
 "unsigned int" => "ffi_type_uint32",
 "int" => "ffi_type_sint32",
 "unsigned long long" => "ffi_type_uint64",
 "long long" => "ffi_type_sint64",
 "uint32_t" => "ffi_type_uint32",
 "cuuint32_t" => "ffi_type_uint32",
 "uint64_t" => "ffi_type_uint64",
 "cuuint64_t" => "ffi_type_uint64",
 "float" => "ffi_type_float",
 "double" => "ffi_type_double",
 "size_t" => "ffi_type_pointer",
 "CUdevice" => "ffi_type_uint32",
 "CUdeviceptr_v1" => "ffi_type_uint32",
 "CUdeviceptr" => "ffi_type_pointer",
 "CUtexObject" => "ffi_type_uint64",
 "CUsurfObject" => "ffi_type_uint64",
}

CUDA_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

CUDA_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

CUDA_FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

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
        when *CUDA_STRUCT_TYPES, *CUDA_UNION_TYPES
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
      when *CUDA_OBJECTS, *CUDA_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when "CUdeviceptr"
        ev.macro = :ctf_integer_hex
        ev.type = name
      when *CUDA_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *CUDA_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *CUDA_STRUCT_TYPES, *CUDA_UNION_TYPES
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
        ev.type = CUDA_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *CUDA_OBJECTS, *CUDA_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *CUDA_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *CUDA_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *CUDA_STRUCT_TYPES, *CUDA_UNION_TYPES
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
        res = CUDA_STRUCT_MAP[res.type.type.name].find { |m| m.name == n }
        return nil unless res
      }
      return res
    end
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

class InString < MetaParameter
  prepend In
  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    ev.macro = :ctf_string
    ev.name = "#{name}_val"
    ev.expression = sanitize_expression("#{name}")
    @lttng_in_type = ev
  end
end

class OutString < MetaParameter
  prepend Out
  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    ev.macro = :ctf_string
    ev.name = "#{name}_val"
    ev.expression = sanitize_expression("#{name}")
    @lttng_out_type = ev
  end
end

class OutPtrString < MetaParameter
  prepend Out
  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    ev.macro = :ctf_string
    ev.name = "#{name}_val_val"
    ev.expression = sanitize_expression("*#{name}")
    @lttng_out_type = ev
  end
end

class ScalarMetaParameter < MetaParameter
  attr_reader :type

  def initialize(command, name, type = nil)
    super(command, name)
    @type = type
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name} #{command.parameters}!" unless a
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
      lttngt.expression = sanitize_expression("#{name}")
      checks = check_for_null("#{name}")
      lttngt.length = sanitize_expression("#{lttngt.length}", checks)
      lttngt.length_type = "size_t"
    elsif type
      checks = check_for_null("#{name}")
      lttngt.expression = sanitize_expression("*(#{YAMLCAst::Pointer::new(type: st)})#{name}", checks)
    else
      checks = check_for_null("#{name}")
      lttngt.expression = sanitize_expression("*#{name}", checks)
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

class FixedArrayMetaParameter < MetaParameter
  attr_reader :size
  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    checks = check_for_null("#{name}")
    if t.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: size, length_type: nil)
    lttngt.name = name + "_vals"
    lttngt.expression = sanitize_expression("#{name}")
    @lttng_type = lttngt
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

class ArrayByRefMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.kind_of?(YAMLCAst::Pointer)
    raise "Type is not a pointer to an array: #{t}!" if !t.type.kind_of?(YAMLCAst::Pointer)
    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s
    if s.type.kind_of?(YAMLCAst::Pointer)
      checks = check_for_null("#{size}") + check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("*#{size}", checks)
      st = "#{s.type.type}"
    else
      checks = check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("#{size}", checks)
      st = "#{s.type}"
    end
    if t.type.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: sz, length_type: st)
    lttngt.name = name + "_val_vals"
    lttngt.expression = sanitize_expression("*#{name}")
    @lttng_type = lttngt
  end
end

class OutArrayByRef < ArrayByRefMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
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

class InFixedArray < FixedArrayMetaParameter
  prepend In
  def initialize(command, name, size)
    super
    @lttng_in_type = @lttng_type
  end
end

class OutFixedArray < FixedArrayMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
  end
end

class TracepointParameter
  attr_reader :name
  attr_reader :type
  attr_reader :init
  attr_reader :after

  def initialize(name, type, init, after: false)
    @name = name
    @type = type
    @init = init
    @after = after
  end

  def after?
    @after
  end
end

class OutNullArray < OutArray
  def initialize(command, name)
    sname = "_#{name.split("->").join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF, after: true)
  #{sname} = 0;
  if(#{checks.join(" && ")}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

class InNullArray < InArray
  def initialize(command, name)
    sname = "_#{name.split("->").join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF)
  #{sname} = 0;
  if(#{checks.join(" && ")}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

class OutLTTng < MetaParameter
  prepend Out
  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]
    super(command, name)
    @lttng_out_type = LTTng::TracepointField::new(*args)
  end
end

class InLTTng < MetaParameter
  prepend In
  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]
    super(command, name)
    @lttng_in_type = LTTng::TracepointField::new(*args)
  end
end

def register_meta_parameter(method, type, *args)
  META_PARAMETERS[method].push [type, args]
end

def register_meta_struct(method, name, type)
  raise "Unknown struct: #{type}!" unless CUDA_STRUCT_TYPES.include?(type)
  CUDA_STRUCT_MAP[type].each { |m|
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

$cuda_meta_parameters = YAML::load_file(File.join(SRC_DIR,"cuda_meta_parameters.yaml"))
$cuda_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$cuda_exports_meta_parameters = YAML::load_file(File.join(SRC_DIR,"cuda_exports_meta_parameters.yaml"))
$cuda_exports_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$cuda_commands = cuda_funcs_e.collect { |func|
  Command::new(func)
}

$cuda_exports_commands = cuda_exports_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

CUDA_POINTER_NAMES = ($cuda_commands +
                      $cuda_exports_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, extra);
EOF

[ "cuLaunchKernel",
  "cuLaunchKernel_ptsz",
  "cuLaunchKernelEx",
  "cuLaunchKernelEx_ptsz" ].each { |m|
  register_prologue m, dump_args
}

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, NULL);
EOF

[ "cuLaunchCooperativeKernel",
  "cuLaunchCooperativeKernel_ptsz" ].each { |m|
  register_prologue m, dump_args
}

dump_args = <<EOF
  if (nodeParams) {
    _dump_kernel_args(nodeParams->func, nodeParams->kernelParams, nodeParams->extra);
  }
EOF

[ "cuGraphAddKernelNode",
  "cuGraphExecKernelNodeSetParams" ].each { |m|
  register_prologue m, dump_args
}

register_prologue "cuLaunchCooperativeKernelMultiDevice", <<EOF
  if (launchParamsList) {
    for( unsigned int _i = 0; _i < numDevices; _i++) {
      _dump_kernel_args(launchParamsList[_i].function, launchParamsList[_i].kernelParams, NULL);
    }
  }
EOF

register_epilogue "cuGraphKernelNodeGetParams", <<EOF
  if (_retval == CUDA_SUCCESS && nodeParams) {
    _dump_kernel_args(nodeParams->func, nodeParams->kernelParams, nodeParams->extra);
  }
EOF

# Profiling

profiling_start = lambda { |stream|
  <<EOF
  CUevent _hStart = NULL;
  if (_do_profile)
    _hStart = _create_record_event(#{stream});
EOF
}

profiling_start_no_stream = profiling_start.call("NULL")
profiling_start_stream = profiling_start.call("hStream")

profiling_start_config = <<EOF
  CUevent _hStart = NULL;
  if (_do_profile && config)
    _hStart = _create_record_event(config->hStream);
EOF

profiling_stop = lambda { |stream|
  <<EOF
  if (_do_profile)
    _event_profile(_retval, _hStart, #{stream});
EOF
}

profiling_stop_no_stream = profiling_stop.call("NULL")
profiling_stop_stream = profiling_stop.call("hStream")

profiling_stop_config = <<EOF
  if (_do_profile && config)
    _event_profile(_retval, _hStart, config->hStream);
EOF

stream_commands = []
no_stream_commands = []
mem_commands = $cuda_commands.select { |c| c.name.match(/cuMemcpy|cuMemset/) }
mem_stream_commands = mem_commands.select { |c| c.name.match(/Async/) }
mem_no_stream_commands = mem_commands - mem_stream_commands
stream_commands += mem_stream_commands.collect(&:name)
no_stream_commands += mem_no_stream_commands.collect(&:name)

stream_commands += %w(
  cuMemPrefetchAsync
  cuMemPrefetchAsync_ptsz
  cuLaunchGridAsync
  cuLaunchKernel
  cuLaunchCooperativeKernel
  cuLaunchHostFunc
  cuLaunchKernel_ptsz
  cuLaunchCooperativeKernel_ptsz
  cuLaunchHostFunc_ptsz
)

no_stream_commands += %w(
  cuLaunch
  cuLaunchGrid
)

config_commands = %w(
  cuLaunchKernelEx
  cuLaunchKernelEx_ptsz
)

stream_commands.each { |m|
  register_prologue m, profiling_start_stream
  register_epilogue m, profiling_stop_stream
}

no_stream_commands.each { |m|
  register_prologue m, profiling_start_no_stream
  register_epilogue m, profiling_stop_no_stream
}

config_commands.each { |m|
  register_prologue m, profiling_start_config
  register_epilogue m, profiling_stop_config
}

# if a context is to be destroyed we must attempt to get profiling event results
[ "cuCtxDestroy",
  "cuCtxDestroy_v2" ].each { |m|
  register_prologue m, <<EOF
  if (ctx) {
    _context_event_cleanup(ctx);
  }
EOF
}

register_epilogue "cuDevicePrimaryCtxRetain", <<EOF
  if (_do_profile && _retval == CUDA_SUCCESS && *pctx) {
    _primary_context_retain(dev, *pctx);
  }
EOF

[ "cuDevicePrimaryCtxRelease_v2",
  "cuDevicePrimaryCtxRelease" ].each { |m|
  register_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_release(dev);
  }
EOF
}

[ "cuDevicePrimaryCtxReset_v2",
  "cuDevicePrimaryCtxReset" ].each { |m|
  register_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_reset(dev);
  }
EOF
}

# Export tracing
register_epilogue "cuGetExportTable", <<EOF
  if (_do_trace_export_tables && _retval == CUDA_SUCCESS) {
    const void *tmp = _wrap_and_cache_export_table(*ppExportTable, pExportTableId);
    tracepoint(lttng_ust_cuda, cuGetExportTable_exit, ppExportTable, pExportTableId, _retval);
    *ppExportTable = tmp;
    return _retval;
  }
EOF
