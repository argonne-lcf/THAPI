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

provider = :lttng_ust_ze

$ze_api_yaml = YAML::load_file("ze_api.yaml")
$zet_api_yaml = YAML::load_file("zet_api.yaml")
$zes_api_yaml = YAML::load_file("zes_api.yaml")
$zel_api_yaml = YAML::load_file("zel_api.yaml")

$ze_api = YAMLCAst.from_yaml_ast($ze_api_yaml)
$zet_api = YAMLCAst.from_yaml_ast($zet_api_yaml)
$zes_api = YAMLCAst.from_yaml_ast($zes_api_yaml)
$zel_api = YAMLCAst.from_yaml_ast($zel_api_yaml)

ze_funcs_e = $ze_api["functions"]
zet_funcs_e = $zet_api["functions"]
zes_funcs_e = $zes_api["functions"]
zel_funcs_e = $zel_api["functions"]

ze_types_e = $ze_api["typedefs"]
zet_types_e = $zet_api["typedefs"]
zes_types_e = $zes_api["typedefs"]
zel_types_e = $zel_api["typedefs"]

all_types = ze_types_e + zet_types_e + zes_types_e + zel_types_e
all_structs = $ze_api["structs"] + $zet_api["structs"] + $zes_api["structs"] + $zel_api["structs"]

ZE_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    ZE_OBJECTS.push t.name
  end
}
ZE_INT_SCALARS = %w(uintptr_t size_t int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t char)
ZE_FLOAT_SCALARS = %w(float double)
ZE_SCALARS = ZE_INT_SCALARS + ZE_FLOAT_SCALARS

all_types.each { |t| 
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_INT_SCALARS.include?(t.type.name)
    ZE_INT_SCALARS.push t.name
  end
}

ZE_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
ZE_STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name } + [ "zet_core_callbacks_t", "zel_core_callbacks_t" ]
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

INIT_FUNCTIONS = /zeInit|zeLoaderInit/

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
 "uintptr_t" => "ffi_type_pointer",
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
      when *ZE_OBJECTS, *ZE_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
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
        ev.type = ZE_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *ZE_OBJECTS, *ZE_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
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
        res = ZE_STRUCT_MAP[res.type.type.name].find { |m| m.name == n }
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
  def initialize(command, name, size = nil)
    super(command, name)
    a = command[name]
    s = nil
    s = command[size] if size
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    if s
      ev.macro = :ctf_sequence_text
      if s.type.kind_of?(YAMLCAst::Pointer)
        checks = check_for_null("#{size}") + check_for_null("#{name}")
        sz = sanitize_expression("*#{size}", checks)
        st = "#{s.type.type}"
      else
        checks = check_for_null("#{name}")
        sz = sanitize_expression("#{size}", checks)
        st = "#{s.type}"
      end
      ev.type = "char"
      ev.length = sz
      ev.length_type = st
    else
      ev.macro = :ctf_string
    end
    ev.name = "#{name}_val"
    ev.expression = sanitize_expression("#{name}")
    @lttng_out_type = ev
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

$ze_meta_parameters = YAML::load_file(File.join(SRC_DIR, "ze_meta_parameters.yaml"))
$ze_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$zet_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zet_meta_parameters.yaml"))
$zet_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$zes_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zes_meta_parameters.yaml"))
$zes_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$zel_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zel_meta_parameters.yaml"))
$zel_meta_parameters["meta_parameters"].each  { |func, list|
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

$zes_commands = zes_funcs_e.collect { |func|
  Command::new(func)
}

$zel_commands = zel_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ZE_POINTER_NAMES = ($ze_commands + $zet_commands + $zes_commands + $zel_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h

register_epilogue "zeDeviceGet", <<EOF
  if (_do_profile) {
    if (_retval == ZE_RESULT_SUCCESS && phDevices && pCount) {
      for (uint32_t i = 0; i < *pCount; i++)
        _register_ze_device(phDevices[i], hDriver);
    }
  }
EOF

register_epilogue "zeCommandListCreate", <<EOF
  if (_do_profile) {
    if (_retval == ZE_RESULT_SUCCESS && phCommandList && *phCommandList) {
      _register_ze_command_list(*phCommandList, hContext, hDevice);
    }
  }
EOF

register_epilogue "zeCommandListDestroy", <<EOF
  if (_do_profile) {
    if (_retval == ZE_RESULT_SUCCESS && hCommandList) {
      _unregister_ze_command_list(hCommandList);
    }
  }
EOF

register_prologue "zeEventPoolCreate", <<EOF
  ze_event_pool_desc_t _new_desc;
  if (_do_profile && desc) {
    _new_desc = *desc;
    _new_desc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    desc = &_new_desc;
  }
EOF

register_prologue "zeEventDestroy", <<EOF
  if (_do_profile && hEvent) {
    _unregister_ze_event(hEvent);
  }
EOF

register_prologue "zeEventHostReset", <<EOF
  if (_do_profile && hEvent) {
    _unregister_ze_event(hEvent);
  }
EOF

register_epilogue "zeContextDestroy", <<EOF
  if (_do_profile && hContext) {
    _context_cleanup(hContext);
  }
EOF

# WARNING: there seems to be no way to profile if
# zeCommandListAppendEventReset is used or at least
# not very cleanly is used....
profiling_prologue = lambda { |event_name|
  <<EOF
  ze_event_pool_handle_t _pool = NULL;
  if (_do_profile && !#{event_name}) {
    #{event_name} = _get_profiling_event(hCommandList, &_pool);
  }
EOF
}

profiling_epilogue = lambda { |event_name|
  <<EOF
  if (_do_profile && #{event_name}) {
    if (_retval == ZE_RESULT_SUCCESS) {
      _register_ze_event(#{event_name}, hCommandList, _pool);
      tracepoint(lttng_ust_ze_profiling, event_profiling, #{event_name});
    } else if (_pool) {
      ZE_EVENT_DESTROY_PTR(#{event_name});
      ZE_EVENT_POOL_DESTROY_PTR(_pool);
    }
  }
EOF
}

[ "zeCommandListAppendLaunchKernel",
  "zeCommandListAppendBarrier",
  "zeCommandListAppendLaunchCooperativeKernel",
  "zeCommandListAppendLaunchKernelIndirect",
  "zeCommandListAppendLaunchMultipleKernelsIndirect",
  "zeCommandListAppendMemoryRangesBarrier",
  "zeCommandListAppendMemoryCopy",
  "zeCommandListAppendMemoryFill",
  "zeCommandListAppendMemoryCopyFromContext",
  "zeCommandListAppendMemoryCopyRegion",
  "zeCommandListAppendImageCopy",
  "zeCommandListAppendImageCopyRegion",
  "zeCommandListAppendImageCopyToMemory",
  "zeCommandListAppendImageCopyFromMemory",
  "zeCommandListAppendQueryKernelTimestamps",
  "zeCommandListAppendWriteGlobalTimestamp" ].each { |c|
    register_prologue c, profiling_prologue.call("hSignalEvent")
    register_epilogue c, profiling_epilogue.call("hSignalEvent")
}

#WARNING
# zeModuleGetKernelNames, returns an array of strings.
# This is problematic for lttng.
