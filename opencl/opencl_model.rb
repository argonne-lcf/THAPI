require 'nokogiri'
require 'yaml'

HOST_PROFILE = true

WINDOWS = /D3D|DX9/

VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$|OCLICD$/

ABSENT_FUNCTIONS = /^clIcdGetPlatformIDsKHR$|^clCreateProgramWithILKHR$|^clTerminateContextKHR$|^clCreateCommandQueueWithPropertiesKHR$|^clEnqueueMigrateMemObjectEXT$/

EXTENSION_FUNCTIONS = /KHR$|EXT$|GL/

SUPPORTED_EXTENSION_FUNCTIONS = /#{YAML::load_file("supported_extensions.yaml").join("|")}/

INIT_FUNCTIONS = /clGetPlatformIDs|clGetPlatformInfo|clGetDeviceIDs|clCreateContext|clCreateContextFromType|clUnloadPlatformCompiler|clGetExtensionFunctionAddressForPlatform|clGetExtensionFunctionAddress|clGetGLContextInfoKHR/

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

ENUMS = {}
ENUM_PARAM_NAME_MAP = {}
ENUM_TYPES = []

#map = Hash::new { |h, k| h[k] = [] }

doc = Nokogiri::XML(open("cl.xml"))
funcs_e = doc.xpath("//commands/command").reject do |l|
  name = l.search("proto/name").text
  name.match(VENDOR_EXT) || name.match(ABSENT_FUNCTIONS) || name.match(WINDOWS)
end.collect

ext_funcs_e = doc.xpath("//commands/command").select do |l|
  name = l.search("proto/name").text
  name.match(SUPPORTED_EXTENSION_FUNCTIONS)
end.collect

typedef_e = doc.xpath("//types/type").select do |l|
  l["category"] == "define" && l.search("type").size > 0
end.collect

struct_e = doc.xpath("//types/type").select do |l|
  l["category"] == "struct"
end.collect

require_e = doc.xpath("//feature/require").to_a + doc.xpath("//extensions/extension/require").to_a
constants = doc.xpath("//enums/enum").collect { |n|
  if n["value"]
    [n["name"], n["value"]]
  elsif n["bitpos"]
    [n["name"], "(1 << #{n["bitpos"]})"]
  end
}.to_h

CL_OBJECTS = ["cl_platform_id", "cl_device_id", "cl_context", "cl_command_queue", "cl_mem", "cl_program", "cl_kernel", "cl_event", "cl_sampler"]

CL_EXT_OBJECTS = ["cl_GLsync", "CLeglImageKHR", "CLeglDisplayKHR", "CLeglSyncKHR"]

CL_INT_SCALARS = ["unsigned int", "int", "intptr_t", "size_t", "cl_int", "cl_uint", "cl_long", "cl_ulong", "cl_short", "cl_ushort", "cl_char", "cl_uchar"]
CL_FLOAT_SCALARS = ["cl_half", "cl_float", "cl_double"]
CL_FLOAT_SCALARS_MAP = {"cl_half" => "cl_ushort", "cl_float" => "cl_uint", "cl_double" => "cl_ulong"}
CL_BASE_TYPES = CL_INT_SCALARS + CL_FLOAT_SCALARS

CL_TYPE_MAP = typedef_e.collect { |l|
  [l.search("name").text, l.search("type").text]
}.to_h

CL_TYPE_MAP.transform_values! { |v|
  until CL_BASE_TYPES.include? v
    v = CL_TYPE_MAP[v]
  end
  v
}

CL_TYPE_MAP.merge!([["cl_GLint", "int"], ["cl_GLenum", "unsigned int"], ["cl_GLuint", "unsigned int"]].to_h)

FFI_BASE_TYPES = ["ffi_type_uint8", "ffi_type_sint8", "ffi_type_uint16", "ffi_type_sint16", "ffi_type_uint32", "ffi_type_sint32", "ffi_type_uint64", "ffi_type_sint64", "ffi_type_float", "ffi_type_double", "ffi_type_void", "ffi_type_pointer"]
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
 "cl_double" => "double",
 "cl_float" => "float",
 "cl_char" => "int8_t",
 "cl_uchar" => "uint8_t",
 "cl_short" => "int16_t",
 "cl_ushort" => "uint16_t",
 "cl_int" => "int32_t",
 "cl_uint" => "uint32_t",
 "cl_long" => "int64_t",
 "cl_ulong" => "uint64_t",
 "cl_half" => "uint8_t"
}

FFI_TYPE_MAP.merge! typedef_e.collect { |l|
  [l.search("name").text, l.search("type").text]
}.to_h


FFI_TYPE_MAP.transform_values! { |v|
  until FFI_BASE_TYPES.include? v
    v = FFI_TYPE_MAP[v]
    exit unless v
  end
  v
}

class CLXML

  attr_reader :__node

  def initialize(node)
    @__node = node
  end

  def inspect
    str = "#<#{self.class}:#{(object_id << 1).to_s(16)} "
    str << instance_variables.reject { |v| v == :@__node }.collect { |v| "#{v.to_s}=#{instance_variable_get(v).inspect}" }.join(", ")
    str << ">"
    str
  end

end

class Require < CLXML
  attr_reader :comment

  def initialize(node)
    super
    @comment = node["comment"]
  end

  def bitfield?
    @comment.match("bitfield")
  end

  def enums
    @__node.search("enum").collect { |e| e["name"] }
  end

end

enums = YAML::load_file("supported_enums.yaml")

require_e = require_e.collect { |r| Require::new(r) }

enums.each { |e|
  vals = require_e.select { |r|
    r.comment && r.comment.match(e["name"])
  }.collect { |r|
    r.enums
  }.reduce(:+).collect { |v|
   [v, constants[v]]
  }.to_h
  ENUMS[e["name"]] = { "values" => vals, "trace_name" => e["trace_name"], "type_name" => e["type_name"] }
  ENUM_PARAM_NAME_MAP[e["trace_name"]] = e["type_name"]
  ENUM_TYPES.push(e["type_name"] ? e["type_name"] : e["name"])
}
ENUM_TYPES.push "cl_bool"

class Declaration < CLXML
  attr_reader :type
  attr_reader :name

  def initialize(param)
    super
    @name = param.search("name").text
    @type = param.search("type").text
    @type += "*" if decl.match?(/\*\*/)
    @__callback = nil
  end

  def decl
    @__node.children.collect(&:text).join(" ").squeeze(" ")
  end

  def decl_pointer
    @__node.children.collect { |n| "#{n.name == "name" ? "" : n.text}" }.join(" ").squeeze(" ")
  end

  def pointer?
    @__pointer if !@__pointer.nil?
    @__pointer = false
    @__node.children.collect { |n|
      break if n.name == "name"
      if n.text.match("\\*")
        @__pointer = true
        break
      end
    }
    @__pointer
  end

end

class Member < Declaration
  def initialize(command, member, prefix, dir = :start)
    super(member)
    name = "#{prefix}_#{@name}"
    expr = "#{prefix} != NULL ? #{prefix}->#{@name} : 0"
    @dir = dir
    @lttng_type = [:ctf_integer_hex, :intptr_t, name, "(intptr_t)(#{expr})"] if pointer?
    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]
    case t
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_type = [:ctf_integer_hex, :intptr_t, name, "(intptr_t)(#{expr})"]
    when *CL_INT_SCALARS
      @lttng_type = [:ctf_integer, t, name, expr]
    when *CL_FLOAT_SCALARS
      @lttng_type = [:ctf_float, t, name, expr]
    end
   end

   def lttng_in_type
     @dir == :start ? @lttng_type : nil
   end

   def lttng_out_type
     @dir == :start ? nil : @lttng_type
   end

end

CL_STRUCT_MAP = struct_e.collect { |s|
  members = s.search("member")
  [s["name"], members]
}.to_h

CL_STRUCTS = CL_STRUCT_MAP.keys

class Parameter < Declaration

  def initialize(param)
    super
    @__callback = nil
  end

  def callback?
    @__callback if !@__callback.nil?
    @__callback = false
    @__node.children.collect { |n| @__callback = true if n.text.match("CL_CALLBACK") }
    @__callback
  end

  def pointer?
    return true if callback?
    super
  end

  def lttng_in_type
    if pointer?
      return [:ctf_integer_hex, :intptr_t, @name, "(intptr_t)#{@name}"]
    end
    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]
    if ENUM_TYPES.include? @type
      return [:ctf_enum, :lttng_ust_opencl, @type, t, @name, @name]
    else
      case t
      when *CL_OBJECTS, *CL_EXT_OBJECTS
        return [:ctf_integer_hex, :intptr_t, @name, "(intptr_t)#{@name}"]
      when *CL_INT_SCALARS
        return [:ctf_integer, t, @name, @name]
      when *CL_FLOAT_SCALARS
        return [:ctf_float, t, @name, @name]
      end
    end
    nil
  end

  def void?
    decl.strip == "void"
  end

  def lttng_out_type
    nil
  end

  def ffi_type
    return "ffi_type_pointer" if pointer? || CL_OBJECTS.include?(type) || CL_EXT_OBJECTS.include?(type)
    return "ffi_type_void" if void?
    return FFI_TYPE_MAP[type]
  end

end

class Prototype < CLXML

  attr_reader :return_type
  attr_reader :name

  def has_return_type?
    return_type != "void"
  end

  def ffi_return_type
    return "ffi_type_void" unless has_return_type?
    return "ffi_type_pointer" if return_type.match(/\*/) || CL_OBJECTS.include?(return_type) || CL_EXT_OBJECTS.include?(return_type)
    FFI_TYPE_MAP[return_type]
  end

  def initialize(proto)
    super
    @name = proto.search("name").text
    @return_type = @__node.children.reject { |c| c.name == "name" }.collect(&:text).join(" ").squeeze(" ").strip
  end

  def decl
    @__node.children.collect { |n| "#{n.name == "name" ? "CL_API_CALL " : ""}#{n.text}" }.join(" ").squeeze(" ")
  end

  def decl_pointer(type: false)
    @__node.children.collect { |n| "#{n.name == "name" ? "(CL_API_CALL *#{type ? pointer_type_name : pointer_name})" : n.text}" }.join(" ").squeeze(" ")
  end

  def pointer_name
    @name + "_ptr"
  end

  def ffi_function_name
    @name + "_ffi"
  end

  def pointer_type_name
    @name + "_t"
  end

  def lttng_return_type
    if @return_type.match("\\*")
      return [:ctf_integer_hex, :intptr_t, "_retval", "(intptr_t)_retval"]
    end
    case @return_type
    when "cl_int"
      return [:ctf_enum, :lttng_ust_opencl, :cl_errcode, :cl_int, "errcode_ret_val", "_retval"]
    when *CL_OBJECTS
      return [:ctf_integer_hex, :intptr_t, @return_type.gsub(/^cl_/,""), "(intptr_t)_retval"]
    when *CL_EXT_OBJECTS
      return [:ctf_integer_hex, :intptr_t, @return_type.gsub(/^CL/,"").gsub(/KHR$/,""), "(intptr_t)_retval"]
    when "void*"
      return [:ctf_integer_hex, :intptr_t, "ret_ptr", "(intptr_t)_retval"]
    end
    nil
  end

end

class MetaParameter
  def initialize(command, name)
    @command = command
    @name = name
  end

  def lttng_array_type_broker(type, name, size, stype = nil)
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    if stype
      stype = CL_TYPE_MAP[stype] if CL_TYPE_MAP[stype]
      lttng_arr_type = "sequence"
      lttng_args = [ stype, "#{name} == NULL ? 0 : #{size}" ]
    else
      lttng_arr_type = "array"
      lttng_args = [ size ]
    end
    expr = name
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      lttng_type = ["ctf_#{lttng_arr_type}_hex", :intptr_t]
    when *CL_INT_SCALARS
      lttng_type = ["ctf_#{lttng_arr_type}", type]
    when *CL_FLOAT_SCALARS
      lttng_type = ["ctf_#{lttng_arr_type}_hex", CL_FLOAT_SCALARS_MAP[type]]
    when *CL_STRUCTS
      lttng_type = ["ctf_#{lttng_arr_type}_text", :uint8_t]
    when ""
      lttng_type = ["ctf_#{lttng_arr_type}_text", :uint8_t]
    when /\*/
      lttng_type = ["ctf_#{lttng_arr_type}_hex", :intptr_t]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
    lttng_type += [ name+"_vals", expr ]
    lttng_type += lttng_args
  end

  def lttng_in_type
    nil
  end

  def lttng_out_type
    nil
  end
end

class OutMetaParameter < MetaParameter
  def lttng_out_type
    @lttng_out_type
  end
end

class InMetaParameter < MetaParameter
  def lttng_in_type
    @lttng_in_type
  end
end

class OutScalar < OutMetaParameter
  def initialize(command, name)
    super
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type.gsub("*", "")
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    if ENUM_PARAM_NAME_MAP[name]
      @lttng_out_type = [:ctf_enum, :lttng_ust_opencl, ENUM_PARAM_NAME_MAP[name], type, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
    else
      case type
      when *CL_OBJECTS, *CL_EXT_OBJECTS
        @lttng_out_type = [:ctf_integer_hex, :intptr_t, name+"_val", "(intptr_t)(#{name} == NULL ? 0 : *#{name})"]
      when *CL_INT_SCALARS
        @lttng_out_type = [:ctf_integer, type, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
      when *CL_FLOAT_SCALARS
        @lttng_out_type = [:ctf_float, type, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
      when ""
        @lttng_out_type = [:ctf_integer_hex, :intptr_t, name+"_val", "(intptr_t)(#{name} == NULL ? 0 : *#{name})"]
      else
        raise "Unknown Type: #{type.inspect}!"
      end
    end
  end
end

class InFixedArray  < InMetaParameter
  def initialize(command, name, count)
    super(command, name)
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    @lttng_in_type = lttng_array_type_broker(type, name, count)
  end
end

class OutArray < OutMetaParameter
  def initialize(command, name, sname = "num_entries")
    super(command, name)
    @sname = sname
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    raise "Couldn't find variable #{sname} for #{command.prototype.name}!" unless command[sname]
    stype = command[sname].type
    @lttng_out_type = lttng_array_type_broker(type, name, sname, stype)
  end
end

class InArray < InMetaParameter
  def initialize(command, name, sname = "num_entries")
    super(command, name)
    @sname = sname
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    raise "Couldn't find variable #{sname} for #{command.prototype.name}!" unless command[sname]
    stype = command[sname].type
    @lttng_in_type = lttng_array_type_broker(type, name, sname, stype)
  end
end

class DeviceFissionPropertyList < InArray
  def initialize(command, name)
    sname = "_#{name}_size"
    type = command[name].type
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF)
  #{sname} = 0;
  if(#{name} != NULL) {
    while(#{name}[#{sname}++] != CL_PROPERTIES_LIST_END_EXT) {
      switch(#{name}[#{sname}]) {
      case CL_DEVICE_PARTITION_EQUALLY_EXT:
      case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT:
        #{sname}++; //value
        break;
      case CL_DEVICE_PARTITION_BY_COUNTS_EXT:
        while(#{name}[#{sname}++] != CL_PARTITION_BY_COUNTS_LIST_END_EXT);
        break;
      case CL_DEVICE_PARTITION_BY_NAMES_EXT:
        while(#{name}[#{sname}] != (#{type})CL_PARTITION_BY_NAMES_LIST_END_EXT);
        break;
      }
    }
  }
EOF
    super(command, name, sname)
  end
end

class InNullArray < InArray
  def initialize(command, name)
    sname = "_#{name}_size"
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF)
  #{sname} = 0;
  if(#{name} != NULL) {
    while(#{name}[#{sname}++] != 0);
  }
EOF
    super(command, name, sname)
  end
end

class InString < InMetaParameter
  def initialize(command, name)
    super
    @lttng_in_type = [:ctf_string, name+"_val", name]
  end
end

class AutoMetaParameter
  def self.create_if_match(command)
    nil
  end
end

class EventWaitList < AutoMetaParameter
  def self.create_if_match(command)
    el = command.parameters.find { |p| p.name == "event_wait_list" }
    if el
      return InArray::new(command, "event_wait_list", "num_events_in_wait_list")
    end
    nil
  end
end

class AutoOutScalar
  def self.create(name)
    str = <<EOF
    Class::new(AutoMetaParameter) do
      def self.create_if_match(command)
        par = command.parameters.find { |p| p.name == "#{name}" && p.pointer? }
        if par
          return OutScalar::new(command, "#{name}")
        end
        nil
      end
    end
EOF
    eval str
  end
end

class ParamValue < AutoMetaParameter
  def self.create_if_match(command)
    pv = command.parameters.find { |p| p.name == "param_value" }
    if pv
      return OutArray::new(command, "param_value", "param_value_size")
    end
    nil
  end
end

class TracepointParameter
  attr_reader :name
  attr_reader :type
  attr_reader :init

  def initialize(name, type, init)
    @name = name
    @type = type
    @init = init
  end
end

ErrCodeRet = AutoOutScalar::create("errcode_ret")

ParamValueSizeRet = AutoOutScalar::create("param_value_size_ret")

Event = AutoOutScalar::create("event")

def register_meta_parameter(method, type, *args)
  raise "Unknown method: #{method}!" unless OPENCL_COMMAND_NAMES.include?(method) || OPENCL_EXTENSION_COMMAND_NAMES.include?(method)
  META_PARAMETERS[method].push [type, args]
end

def register_meta_struct(method, name, type)
  raise "Unknown method: #{method}!" unless OPENCL_COMMAND_NAMES.include?(method) || OPENCL_EXTENSION_COMMAND_NAMES.include?(method)
  raise "Unknown struct: #{type}!" unless CL_STRUCTS.include?(type)
  CL_STRUCT_MAP[type].each { |m|
    META_PARAMETERS[method].push [Member, [m, name]]
  }
end


def register_prologue(method, code)
  raise "Unknown method: #{method}!" unless OPENCL_COMMAND_NAMES.include?(method) || OPENCL_EXTENSION_COMMAND_NAMES.include?(method)
  PROLOGUES[method].push(code)
end

def register_epilogue(method, code)
  raise "Unknown method: #{method}!" unless OPENCL_COMMAND_NAMES.include?(method) || OPENCL_EXTENSION_COMMAND_NAMES.include?(method)
  EPILOGUES[method].push(code)
end

AUTO_META_PARAMETERS = [EventWaitList, ErrCodeRet, ParamValueSizeRet, ParamValue, Event]
META_PARAMETERS = Hash::new { |h, k| h[k] = [] }
PROLOGUES = Hash::new { |h, k| h[k] = [] }
EPILOGUES = Hash::new { |h, k| h[k] = [] }

class Command < CLXML

  attr_reader :prototype
  attr_reader :parameters
  attr_reader :tracepoint_parameters
  attr_reader :meta_parameters
  attr_reader :prologues
  attr_reader :epilogues

  def initialize( command )
    super
    @prototype = Prototype::new( command.search("proto" ) )
    @parameters = command.search("param").collect { |p| Parameter::new(p) }
    @tracepoint_parameters = []
    @meta_parameters = AUTO_META_PARAMETERS.collect { |klass| klass.create_if_match(self) }.compact
    @meta_parameters += META_PARAMETERS[@prototype.name].collect { |type, args|
      type::new(self, *args)
    }
    @extension = @prototype.name.match(EXTENSION_FUNCTIONS)
    @init      = @prototype.name.match(INIT_FUNCTIONS)
    @prologues = PROLOGUES[@prototype.name]
    @epilogues = EPILOGUES[@prototype.name]
  end

  def [](name)
    res = @parameters.find { |p| p.name == name }
    return res if res
    @tracepoint_parameters.find { |p| p.name == name }
  end

  def decl
    "CL_API_ENTRY " + @prototype.decl + "(" + @parameters.collect(&:decl).join(", ") + ")"
  end

  def decl_pointer(type: false)
    "CL_API_ENTRY " + @prototype.decl_pointer(type: type) + "(" + @parameters.collect(&:decl_pointer).join(", ") + ")"
  end

  def decl_ffi_wrapper
    "void #{@prototype.ffi_function_name}(ffi_cif *cif, #{@prototype.return_type} *ffi_ret, void** args, #{@prototype.pointer_type_name} #{@prototype.pointer_name})"
  end

  def event?
    returns_event? || @parameters.find { |p| p.name == "event" && p.pointer? }
  end

  def returns_event?
    prototype.return_type == "cl_event"
  end

  def extension?
    return !!@extension
  end

  def init?
    return !!@init
  end

  def void_parameters?
    @parameters.size == 1 && @parameters.first.void?
  end

end

OPENCL_COMMAND_NAMES = funcs_e.collect { |c| Prototype::new( c.search("proto" ) ) }.collect { |p| p.name }
OPENCL_EXTENSION_COMMAND_NAMES = ext_funcs_e.collect { |c| Prototype::new( c.search("proto" ) ) }.collect { |p| p.name }

meta_parameters = YAML::load_file("opencl_meta_parameters.yaml")
meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

meta_parameters["meta_structs"].each { |func, list|
  list.each { |args|
    register_meta_struct func, *args
  }
}

$opencl_commands = funcs_e.collect { |func|
  Command::new(func)
}

$opencl_extension_commands = ext_funcs_e.collect { |func|
  Command::new(func)
}

$opencl_commands.each { |c|
  eval "$#{c.prototype.name} = c"
}

$opencl_extension_commands.each { |c|
  eval "$#{c.prototype.name} = c"
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

OPENCL_POINTER_NAMES = ($opencl_commands.collect { |c|
  [c, upper_snake_case(c.prototype.pointer_name)]
} + $opencl_extension_commands.collect { |c|
  [c, c.prototype.pointer_name]
}).to_h

buffer_create_info = InMetaParameter::new($clCreateSubBuffer, "buffer_create_info")
buffer_create_info.instance_variable_set(:@lttng_in_type, [:ctf_sequence_hex, :uint8_t, "buffer_create_info_vals", "buffer_create_info", "size_t", "buffer_create_info == NULL ? 0 : (buffer_create_type == CL_BUFFER_CREATE_TYPE_REGION ? sizeof(cl_buffer_region) : 0)"])

$clCreateSubBuffer.meta_parameters.push buffer_create_info


($opencl_commands+$opencl_extension_commands).each { |c|
  if c.prototype.name.match "clEnqueue"
    c.prologues.push <<EOF
  int64_t _enqueue_counter = 0;
  if (do_dump) {
    pthread_mutex_lock(&enqueue_counter_mutex);
    _enqueue_counter = enqueue_counter;
    enqueue_counter++;
    pthread_mutex_unlock(&enqueue_counter_mutex);
    tracepoint(lttng_ust_opencl_dump, enqueue_counter, _enqueue_counter);
  }
EOF
  end
}

register_epilogue "clCreateKernel", <<EOF
  if (do_dump && _retval != NULL) {
    add_kernel(_retval);
  }
EOF

register_epilogue "clSetKernelArg", <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, arg_size, arg_value, 0);
  }
EOF

register_epilogue "clSetKernelArgSVMPointer", <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, sizeof(arg_value), arg_value, 1);
  }
EOF

register_epilogue "clSVMAlloc", <<EOF
  if (do_dump && _retval != NULL) {
    add_svmptr(_retval, size);
  }
EOF

register_prologue "clSVMFree", <<EOF
  if (do_dump && svm_pointer != NULL) {
    remove_svmptr(svm_pointer);
  }
EOF

str = <<EOF
  int _dump_release_events = 0;
  int _dump_release_event = 0;
  cl_event extra_event;
  if (do_dump && command_queue != NULL && kernel != NULL && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    cl_command_queue_properties properties;
    #{OPENCL_POINTER_NAMES[$clGetCommandQueueInfo]}(command_queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &properties, NULL);
    _dump_release_events = dump_kernel_args(command_queue, kernel, _enqueue_counter, properties, &num_events_in_wait_list, (cl_event **)&event_wait_list);
    if (properties | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE && event == NULL) {
      event = &extra_event;
      _dump_release_event = 1;
    }
  }
EOF
register_prologue "clEnqueueNDRangeKernel", str
register_prologue "clEnqueueNDRangeKernelINTEL", str

str = <<EOF
  if (do_dump && _dump_release_events) {
    for (cl_uint event_index = 0; event_index < num_events_in_wait_list; event_index++) {
      #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(event_wait_list[event_index]);
    }
    free((void *)event_wait_list);
  }
EOF
register_epilogue "clEnqueueNDRangeKernel", str
register_epilogue "clEnqueueNDRangeKernelINTEL", str

register_prologue "clCreateBuffer", <<EOF
  if (do_dump) {
    flags &= ~CL_MEM_HOST_WRITE_ONLY;
    flags &= ~CL_MEM_HOST_NO_ACCESS;
  }
EOF

register_epilogue "clCreateBuffer", <<EOF
  if (do_dump && _retval != NULL) {
    add_buffer(_retval, size);
  }
EOF

register_prologue "clCreateCommandQueue", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    properties |= CL_QUEUE_PROFILING_ENABLE;
  }
EOF

register_prologue "clCreateCommandQueueWithProperties", <<EOF
  cl_queue_properties *_profiling_properties = NULL;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    int _found_queue_properties = 0;
    int _queue_properties_index = 0;
    int _properties_count = 0;
    if (properties) {
      while(properties[_properties_count]) {
        if (properties[_properties_count] == CL_QUEUE_PROPERTIES){
          _found_queue_properties = 1;
          _queue_properties_index = _properties_count;
        }
        _properties_count += 2;
      }
      _properties_count++;
      if (!_found_queue_properties)
        _properties_count +=2;
    } else
      _properties_count = 3;
    _profiling_properties = (cl_queue_properties *)malloc(_properties_count*sizeof(cl_queue_properties));
    if (_profiling_properties) {
      if (properties) {
        int _i = 0;
        while(properties[_i]) {
          _profiling_properties[_i] = properties[_i];
          _profiling_properties[_i+1] = properties[_i+1];
          _i += 2;
        }
        if (_found_queue_properties) {
          _profiling_properties[_queue_properties_index+1] |= CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        } else {
          _profiling_properties[_i++] = CL_QUEUE_PROPERTIES;
          _profiling_properties[_i++] = CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        }
      } else {
        _profiling_properties[0] = CL_QUEUE_PROPERTIES;
        _profiling_properties[1] = CL_QUEUE_PROFILING_ENABLE;
        _profiling_properties[2] = 0;
      }
      properties = _profiling_properties;
    }
  }
EOF

register_epilogue "clCreateCommandQueueWithProperties", <<EOF
  if (_profiling_properties) free(_profiling_properties);
EOF

register_prologue "clCreateProgramWithSource", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_string) && strings != NULL) {
    cl_uint index;
    for (index = 0; index < count; index++) {
      size_t length = 0;
      char path[sizeof(SOURCE_TEMPLATE)];
      strncpy(path, SOURCE_TEMPLATE, sizeof(path));
      if ( strings[index] != NULL ) {
        if (lengths == NULL || lengths[index] == 0)
          length = strlen(strings[index]);
        else
          length = lengths[index];
      }
      create_file_and_write(path, length, strings[index]);
      do_tracepoint(lttng_ust_opencl_source, program_string, index, length, path);
    }
  }
EOF

register_prologue "clCreateProgramWithBinary", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_binary) && binaries != NULL && lengths != NULL) {
    cl_uint index;
    for (index = 0; index < num_devices; index++) {
      char path[sizeof(BIN_SOURCE_TEMPLATE)];
      strncpy(path, BIN_SOURCE_TEMPLATE, sizeof(path));
      create_file_and_write(path, lengths[index], binaries[index]);
      do_tracepoint(lttng_ust_opencl_source, program_binary, index, lengths[index], path);
    }
  }
EOF

register_prologue "clCreateProgramWithIL", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_il) && il != NULL) {
    char path[sizeof(IL_SOURCE_TEMPLATE)];
    strncpy(path, IL_SOURCE_TEMPLATE, sizeof(path));
    create_file_and_write(path, length, il);
    do_tracepoint(lttng_ust_opencl_source, program_il, length, path);
  }
EOF

register_prologue "clCreateProgramWithILKHR", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_il) && il != NULL) {
    char path[sizeof(IL_SOURCE_TEMPLATE)];
    strncpy(path, IL_SOURCE_TEMPLATE, sizeof(path));
    create_file_and_write(path, length, il);
    do_tracepoint(lttng_ust_opencl_source, program_il, length, path);
  }
EOF

str = <<EOF
  int _free_options = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info)) {
    struct opencl_version version = {1, 0};
    get_program_platform_version(program, &version);
    if (compare_opencl_version(&version, &opencl_version_1_2) >= 0) {
      if (options) {
        if (!strstr(options, "-cl-kernel-arg-info")) {
          size_t sz = strlen(options) + strlen("-cl-kernel-arg-info") + 2;
          char * new_options = (char *)malloc(sz);
          if (new_options) {
            snprintf(new_options, sz, "%s %s", options, "-cl-kernel-arg-info");
            _free_options = 1;
            options = new_options;
          }
        }
      } else {
        options = "-cl-kernel-arg-info";
      }
    }
  }
EOF
register_prologue "clBuildProgram", str
register_prologue "clCompileProgram", str
register_prologue "clLinkProgram", <<EOF
  int _free_options = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && input_programs && num_input_programs > 0) {
    struct opencl_version version = {1, 0};
    get_program_platform_version(*input_programs, &version);
    if (compare_opencl_version(&version, &opencl_version_1_2) >= 0) {
      if (options) {
        if (!strstr(options, "-cl-kernel-arg-info")) {
          size_t sz = strlen(options) + strlen("-cl-kernel-arg-info") + 2;
          char * new_options = (char *)malloc(sz);
          if (new_options) {
            snprintf(new_options, sz, "%s %s", options, "-cl-kernel-arg-info");
            _free_options = 1;
            options = new_options;
          }
        }
      } else {
        options = "-cl-kernel-arg-info";
      }
    }
  }
EOF

str = <<EOF
  if (_free_options)
    free((char *)options);
EOF
register_epilogue "clBuildProgram", str
register_epilogue "clCompileProgram", str
register_prologue "clLinkProgram", str

l = lambda { |func, name: "pfn_notify", extra_conditions: nil|
  register_prologue func, <<EOF
  struct #{func}_callback_payload *_payload = NULL;
  if ((tracepoint_enabled(lttng_ust_opencl, #{func}_callback_start)#{extra_conditions ? " || #{extra_conditions.join(" || ")}" : ""}) && #{name}) {
    _payload = (struct #{func}_callback_payload *)malloc(sizeof(struct #{func}_callback_payload));
    _payload->#{name} = #{name};
    _payload->user_data = user_data;
    user_data = (void *)_payload;
    #{name} = &#{func}_callback;
  }
EOF
}
program_conditions = ["tracepoint_enabled(lttng_ust_opencl_build, binaries)", "tracepoint_enabled(lttng_ust_opencl_build, infos)"]
l.call("clBuildProgram", extra_conditions: program_conditions)
l.call("clCompileProgram")
l.call("clLinkProgram")
l.call("clCreateContext")
l.call("clCreateContextFromType")
l.call("clSetMemObjectDestructorCallback")
l.call("clSetProgramReleaseCallback")
l.call("clSetEventCallback")
l.call("clEnqueueSVMFree", name: "pfn_free_func")

str = <<EOF
  if (_payload && _retval != CL_SUCCESS)
    free(_payload);
EOF
register_epilogue "clBuildProgram", str
register_epilogue "clCompileProgram", str
register_epilogue "clSetMemObjectDestructorCallback", str
register_epilogue "clSetProgramReleaseCallback", str
register_epilogue "clSetEventCallback", str
register_epilogue "clEnqueueSVMFree", str
str = <<EOF
  if (_payload && !_retval)
    free(_payload);
EOF
register_epilogue "clLinkProgram", str
register_epilogue "clCreateContext", str
register_epilogue "clCreateContextFromType", str

str = <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && !pfn_notify) {
    dump_program_binaries(program);
  }
EOF
register_epilogue "clBuildProgram", str
register_epilogue "clCompileProgram", str
register_epilogue "clLinkProgram", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && _retval && !pfn_notify) {
    dump_program_binaries(_retval);
  }
EOF

str = <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && !pfn_notify) {
    dump_program_build_infos(program);
  }
EOF
register_epilogue "clBuildProgram", str
register_epilogue "clCompileProgram", str
register_epilogue "clLinkProgram", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && _retval && !pfn_notify) {
    dump_program_build_infos(_retval);
  }
EOF

register_epilogue "clCreateKernel", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info)) {
    dump_kernel_info(_retval);
  }
EOF

register_epilogue "clCreateKernel", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval != NULL) {
    dump_kernel_arguments(program, _retval);
  }
EOF

register_prologue "clCreateKernelsInProgram", <<EOF
  cl_uint n_k = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

register_epilogue "clCreateKernelsInProgram", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_info(kernels[i]);
    }
  }
EOF

register_prologue "clCreateKernelsInProgram", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

register_epilogue "clCreateKernelsInProgram", <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_arguments(program, kernels[i]);
    }
  }
EOF

str = $opencl_commands.select{ |c| c.extension? }.collect { |c|
  <<EOF
  if (strcmp(func_name, "#{c.prototype.name}") == 0) {
    tracepoint(lttng_ust_opencl, clGetExtensionFunctionAddressForPlatform_stop, platform, func_name, (void *)(intptr_t)#{OPENCL_POINTER_NAMES[c]}#{HOST_PROFILE ? ", 0" : ""});
    return (void *)(intptr_t)(&#{c.prototype.name});
  }
EOF
}.join(<<EOF)
  else
EOF

register_prologue "clGetExtensionFunctionAddressForPlatform", str

str = $opencl_commands.select{ |c| c.extension? }.collect { |c|
  <<EOF
  if (strcmp(func_name, "#{c.prototype.name}") == 0) {
    tracepoint(lttng_ust_opencl, clGetExtensionFunctionAddress_stop, func_name, (void *)(intptr_t)#{OPENCL_POINTER_NAMES[c]}#{HOST_PROFILE ? ", 0" : ""});
    return (void *)(intptr_t)(&#{c.prototype.name});
  }
EOF
}.join(<<EOF)
  else
EOF

register_prologue "clGetExtensionFunctionAddress", str


register_extension_callbacks = lambda { |ext_method|

  str = <<EOF
  if (_retval != NULL) {
EOF
  str << $opencl_extension_commands.collect { |c|
    sstr = <<EOF
    if (tracepoint_enabled(lttng_ust_opencl, #{c.prototype.name}_start) && strcmp(func_name, "#{c.prototype.name}") == 0) {
      struct opencl_closure *closure = NULL;
      pthread_mutex_lock(&opencl_closures_mutex);
      HASH_FIND_PTR(opencl_closures, &_retval, closure);
      pthread_mutex_unlock(&opencl_closures_mutex);
      if (closure != NULL) {
        tracepoint(lttng_ust_opencl, #{ext_method}_stop,#{ ext_method == "clGetExtensionFunctionAddress" ? "" : " platform,"} func_name, _retval#{HOST_PROFILE ? ", _duration" : ""});
        return closure->c_ptr;
      }
      closure = (struct opencl_closure *)malloc(sizeof(struct opencl_closure) + #{c.parameters.size} * sizeof(ffi_type *));
      closure->types = (ffi_type **)((intptr_t)closure + sizeof(struct opencl_closure));
      if (closure != NULL) {
        closure->closure = ffi_closure_alloc(sizeof(ffi_closure), &(closure->c_ptr));
        if (closure->closure == NULL) {
          free(closure);
        } else {
EOF
    c.parameters.each_with_index { |a, i|
      sstr << <<EOF
         closure->types[#{i}] = &#{a.ffi_type};
EOF
    }
    sstr << <<EOF
          if (ffi_prep_cif(&(closure->cif), FFI_DEFAULT_ABI, #{c.void_parameters? ? 0 : c.parameters.size}, &#{c.prototype.ffi_return_type}, closure->types) == FFI_OK) {
            if (ffi_prep_closure_loc(closure->closure, &(closure->cif), (void (*)(ffi_cif *, void *, void **, void *))#{c.prototype.name}_ffi, _retval, closure->c_ptr) == FFI_OK) {
              pthread_mutex_lock(&opencl_closures_mutex);
              HASH_ADD_PTR(opencl_closures, ptr, closure);
              pthread_mutex_unlock(&opencl_closures_mutex);
              tracepoint(lttng_ust_opencl, #{ext_method}_stop,#{ ext_method == "clGetExtensionFunctionAddress" ? "" : " platform,"} func_name, _retval#{HOST_PROFILE ? ", _duration" : ""});
              return closure->c_ptr;
            }
          }
        }
      }
    }
EOF
  }.join(<<EOF)
    else
EOF
  str << <<EOF
  }
EOF

  register_epilogue ext_method, str
}

register_extension_callbacks.call("clGetExtensionFunctionAddress")
register_extension_callbacks.call("clGetExtensionFunctionAddressForPlatform")


#Create event profiling code
($opencl_commands+$opencl_extension_commands).each { |c|
  if c.event?
    if !c.returns_event?
      c.prologues.push <<EOF
  int _profile_release_event = 0;
  int _event_profiling = 0;
  cl_event _profiling_event;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    if (event == NULL) {
      event = &_profiling_event;
      _profile_release_event = 1;
    }
    _event_profiling = 1;
  }
EOF
      c.epilogues.push <<EOF
  if (_event_profiling) {
    int _set_retval = #{OPENCL_POINTER_NAMES[$clSetEventCallback]}(*event, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, *event);
    if(_profile_release_event) {
      #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(*event);
      event = NULL;
    }
  }
EOF
    else
      c.epilogues.push <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling) ) {
    int _set_retval = #{OPENCL_POINTER_NAMES[$clSetEventCallback]}(_retval, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, _retval);
  }
EOF
    end
  end
}

str = <<EOF
  if (do_dump && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    if (_retval == CL_SUCCESS) {
      cl_event ev = dump_kernel_buffers(command_queue, kernel, _enqueue_counter, event);
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(*event);
        event = NULL;
        if (ev != NULL) {
          #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(ev);
        }
      } else if ( ev != NULL ) {
        if (event != NULL) {
          if (*event != NULL)
            #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(*event);
          *event = ev;
        }
      }
    } else {
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[$clReleaseEvent]}(*event);
        event = NULL;
      }
    }
  }
EOF
register_epilogue "clEnqueueNDRangeKernel", str
register_epilogue "clEnqueueNDRangeKernelINTEL", str

