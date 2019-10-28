require 'nokogiri'
require 'yaml'

provider = :lttng_ust_opencl

WINDOWS = /D3D|DX9/

VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$/

ABSENT_FUNCTIONS = /^clIcdGetPlatformIDsKHR$|^clCreateProgramWithILKHR$|^clTerminateContextKHR$|^clCreateCommandQueueWithPropertiesKHR$|^clEnqueueMigrateMemObjectEXT$/

INIT_FUNCTIONS = /clGetPlatformIDs|clGetPlatformInfo|clGetDeviceIDs|clCreateContext|clCreateContextFromType|clUnloadPlatformCompiler|clGetExtensionFunctionAddressForPlatform|clGetExtensionFunctionAddress|clGetGLContextInfoKHR/

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

#map = Hash::new { |h, k| h[k] = [] }

doc = Nokogiri::XML(open("cl.xml"))
funcs_e = doc.xpath("//commands/command").reject do |l|
  name = l.search("proto/name").text
  name.match(VENDOR_EXT) || name.match(ABSENT_FUNCTIONS) || name.match(WINDOWS)
end.collect

typedef_e = doc.xpath("//types/type").select do |l|
  l["category"] == "define" && l.search("type").size > 0
end.collect

struct_e = doc.xpath("//types/type").select do |l|
  l["category"] == "struct"
end.collect

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
    @lttng_type = [:ctf_integer_hex, :intptr_t, name, expr] if pointer?
    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]
    case t
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_type = [:ctf_integer_hex, :intptr_t, name, expr]
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
      return [:ctf_integer_hex, :intptr_t, @name, @name]
    end
    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]
    case t
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      return [:ctf_integer_hex, :intptr_t, @name, @name]
    when *CL_INT_SCALARS
      return [:ctf_integer, t, @name, @name]
    when *CL_FLOAT_SCALARS
      return [:ctf_float, t, @name, @name]
    end
    nil
  end

  def lttng_out_type
    nil
  end

end

class Prototype < CLXML

  attr_reader :return_type
  attr_reader :name

  def has_return_type?
    return_type != "void"
  end

  def initialize(proto)
    super
    @name = proto.search("name").text
    @return_type = @__node.children.reject { |c| c.name == "name" }.collect(&:text).join(" ").squeeze(" ").strip
  end

  def decl
    @__node.children.collect { |n| "#{n.name == "name" ? "CL_API_CALL " : ""}#{n.text}" }.join(" ").squeeze(" ")
  end

  def decl_pointer
    @__node.children.collect { |n| "#{n.name == "name" ? "(CL_API_CALL *#{pointer_name})" : n.text}" }.join(" ").squeeze(" ")
  end

  def pointer_name
    @name + "_ptr"
  end

  def lttng_return_type
    if @return_type.match("\\*")
      return [:ctf_integer_hex, :intptr_t, "_retval", "_retval"]
    end
    case @return_type
    when "cl_int"
      return [:ctf_integer, :cl_int, "errcode_ret_val", "_retval"]
    when *CL_OBJECTS
      return [:ctf_integer_hex, :intptr_t, @return_type.gsub(/^cl_/,""), "_retval"]
    when *CL_EXT_OBJECTS
      return [:ctf_integer_hex, :intptr_t, @return_type.gsub(/^CL/,"").gsub(/KHR$/,""), "_retval"]
    when "void*"
      return [:ctf_integer_hex, :intptr_t, "ret_ptr", "_retval"]
    end
    nil
  end

end

class MetaParameter
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
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_out_type = [:ctf_integer_hex, :intptr_t, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
    when *CL_INT_SCALARS
      @lttng_out_type = [:ctf_integer, type, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
    when *CL_FLOAT_SCALARS
      @lttng_out_type = [:ctf_float, type, name+"_val", "#{name} == NULL ? 0 : *#{name}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class InFixedArray  < InMetaParameter
  def initialize(command, name, count)
    super(command, name)
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_in_type = [:ctf_array_hex, :intptr_t, name+"_vals", name, count]
    when *CL_INT_SCALARS
      @lttng_in_type = [:ctf_array, type, name+"_vals", name, count]
    when *CL_FLOAT_SCALARS
      @lttng_in_type = [:ctf_array_hex, CL_FLOAT_SCALARS_MAP[type], name+"_vals", name, count]
    when *CL_STRUCTS
      @lttng_in_type = [:ctf_array_text, :uint8_t, name+"_vals", name, count]
    when ""
      @lttng_in_type = [:ctf_array_text, :uint8_t, name+"_vals", name, count]
    when /\*/
      @lttng_in_type = [:ctf_array_hex, :intptr_t, name+"_vals", name, count]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class OutArray < OutMetaParameter
  def initialize(command, name, sname = "num_entries")
    super(command, name)
    @sname = sname
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    raise "Couldn't find variable #{sname} for #{command.prototype.name}!" unless command[sname]
    stype = command[sname].type
    stype = CL_TYPE_MAP[stype] if CL_TYPE_MAP[stype]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_out_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_INT_SCALARS
      @lttng_out_type = [:ctf_sequence, type, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_FLOAT_SCALARS
      @lttng_out_type = [:ctf_sequence_hex, CL_FLOAT_SCALARS_MAP[type], name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_STRUCTS
      @lttng_out_type = [:ctf_sequence_text, :uint8_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}*sizeof(#{type})"]
    when ""
      @lttng_out_type = [:ctf_sequence_text, :uint8_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when /\*/
      @lttng_out_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class InArray < InMetaParameter
  def initialize(command, name, sname = "num_entries")
    super(command, name)
    @sname = sname
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]
    type = command[name].type
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    raise "Couldn't find variable #{sname} for #{command.prototype.name}!" unless command[sname]
    stype = command[sname].type
    stype = CL_TYPE_MAP[stype] if CL_TYPE_MAP[stype]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_in_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_INT_SCALARS
      @lttng_in_type = [:ctf_sequence, type, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_FLOAT_SCALARS
      @lttng_in_type = [:ctf_sequence_hex, CL_FLOAT_SCALARS_MAP[type], name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_STRUCTS
      @lttng_in_type = [:ctf_sequence_text, :uint8_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}*sizeof(#{type})"]
    when ""
      @lttng_in_type = [:ctf_sequence_text, :uint8_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    when /\*/
      @lttng_in_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, stype, "#{name} == NULL ? 0 : #{sname}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class DeviceFissionPropertyList < InArray
  def initialize(command, name)
    sname = "_#{name}_size"
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
        while(#{name}[#{sname}] != CL_PARTITION_BY_NAMES_LIST_END_EXT);
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
  META_PARAMETERS[method].push [type, args]
end

def register_meta_struct(method, name, type)
  raise "Unknown struct: #{type}!" unless CL_STRUCTS.include?(type)
  CL_STRUCT_MAP[type].each { |m|
    META_PARAMETERS[method].push [Member, [m, name]]
  }
end


def register_prologue(method, code)
  PROLOGUES[method].push(code)
end

def register_epilogue(method, code)
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

  def decl_pointer
    "CL_API_ENTRY " + @prototype.decl_pointer + "(" + @parameters.collect(&:decl_pointer).join(", ") + ")"
  end

  def event?
    returns_event? || @parameters.find { |p| p.name == "event" && p.pointer? }
  end

  def returns_event?
    prototype.return_type == "cl_event"
  end

  def init?
    return !!@init
  end

end

register_meta_parameter "clGetPlatformIDs", OutScalar, "num_platforms"
register_meta_parameter "clGetPlatformIDs", OutArray, "platforms"

register_meta_parameter "clGetDeviceIDs", OutScalar, "num_devices"
register_meta_parameter "clGetDeviceIDs", OutArray, "devices"

register_meta_parameter "clCreateContext", InNullArray, "properties"

register_meta_parameter "clCreateContextFromType", InNullArray, "properties"

register_meta_parameter "clEnqueueNDRangeKernel", InArray, "global_work_offset", "work_dim"
register_meta_parameter "clEnqueueNDRangeKernel", InArray, "global_work_size", "work_dim"
register_meta_parameter "clEnqueueNDRangeKernel", InArray, "local_work_size", "work_dim"

register_meta_parameter "clSetCommandQueueProperty", OutScalar, "old_properties"

register_meta_struct    "clCreateImage2D", "image_format", "cl_image_format"

register_meta_struct    "clCreateImage3D", "image_format", "cl_image_format"

register_meta_parameter "clGetSupportedImageFormats", OutScalar, "num_image_formats"
register_meta_parameter "clGetSupportedImageFormats", OutArray, "image_formats"
register_meta_parameter "clCreateProgramWithSource", InArray, "strings", "count"
register_meta_parameter "clCreateProgramWithSource", InArray, "lengths", "count"

register_meta_parameter "clCreateProgramWithBinary", InArray, "device_list", "num_devices"
register_meta_parameter "clCreateProgramWithBinary", InArray, "lengths", "num_devices"
register_meta_parameter "clCreateProgramWithBinary", InArray, "binaries", "num_devices"
register_meta_parameter "clCreateProgramWithBinary", OutArray, "binary_status", "num_devices"

register_meta_parameter "clBuildProgram", InArray, "device_list", "num_devices"
register_meta_parameter "clBuildProgram", InString, "options"

register_meta_parameter "clCreateKernel", InString, "kernel_name"

register_meta_parameter "clCreateKernelsInProgram", OutArray, "kernels", "num_kernels"
register_meta_parameter "clCreateKernelsInProgram", OutScalar, "num_kernels_ret"

register_meta_parameter "clSetKernelArg", InArray, "arg_value", "arg_size"

register_meta_parameter "clWaitForEvents", InArray, "event_list", "num_events"

register_meta_parameter "clEnqueueReadImage", InFixedArray, "origin", 3
register_meta_parameter "clEnqueueReadImage", InFixedArray, "region", 3

register_meta_parameter "clEnqueueWriteImage", InFixedArray, "origin", 3
register_meta_parameter "clEnqueueWriteImage", InFixedArray, "region", 3

register_meta_parameter "clEnqueueCopyImage", InFixedArray, "src_origin", 3
register_meta_parameter "clEnqueueCopyImage", InFixedArray, "dst_origin", 3
register_meta_parameter "clEnqueueCopyImage", InFixedArray, "region", 3

register_meta_parameter "clEnqueueCopyBufferToImage", InFixedArray, "dst_origin", 3
register_meta_parameter "clEnqueueCopyBufferToImage", InFixedArray, "region", 3

register_meta_parameter "clEnqueueMapImage", InFixedArray, "origin", 3
register_meta_parameter "clEnqueueMapImage", InFixedArray, "region", 3
register_meta_parameter "clEnqueueMapImage", OutScalar, "image_row_pitch"
register_meta_parameter "clEnqueueMapImage", OutScalar, "image_slice_pitch"

register_meta_parameter "clEnqueueNativeKernel", InArray, "args", "cb_args"
register_meta_parameter "clEnqueueNativeKernel", InArray, "mem_list", "num_mem_objects"
register_meta_parameter "clEnqueueNativeKernel", InArray, "args_mem_loc", "num_mem_objects"

register_meta_parameter "clEnqueueWaitForEvents", InArray, "event_list", "num_events"

register_meta_parameter "clGetExtensionFunctionAddress", InString, "func_name"

register_meta_parameter "clGetGLObjectInfo", OutScalar, "gl_object_type"
register_meta_parameter "clGetGLObjectInfo", OutScalar, "gl_object_name"

register_meta_parameter "clEnqueueAcquireGLObjects", InArray, "mem_objects", "num_objects"

register_meta_parameter "clEnqueueReleaseGLObjects", InArray, "mem_objects", "num_objects"

register_meta_parameter "clEnqueueReadBufferRect", InFixedArray, "buffer_offset", 3
register_meta_parameter "clEnqueueReadBufferRect", InFixedArray, "host_offset", 3
register_meta_parameter "clEnqueueReadBufferRect", InFixedArray, "region", 3

register_meta_parameter "clEnqueueWriteBufferRect", InFixedArray, "buffer_offset", 3
register_meta_parameter "clEnqueueWriteBufferRect", InFixedArray, "host_offset", 3
register_meta_parameter "clEnqueueWriteBufferRect", InFixedArray, "region", 3

register_meta_parameter "clEnqueueCopyBufferRect", InFixedArray, "src_origin", 3
register_meta_parameter "clEnqueueCopyBufferRect", InFixedArray, "dst_origin", 3
register_meta_parameter "clEnqueueCopyBufferRect", InFixedArray, "region", 3

register_meta_parameter "clCreateSubDevicesEXT", DeviceFissionPropertyList, "properties"
register_meta_parameter "clCreateSubDevicesEXT", OutScalar, "num_devices"
register_meta_parameter "clCreateSubDevicesEXT", OutArray, "out_devices"

register_meta_parameter "clCreateSubDevices", DeviceFissionPropertyList, "properties"
register_meta_parameter "clCreateSubDevices", OutScalar, "num_devices_ret"
register_meta_parameter "clCreateSubDevices", OutArray, "out_devices", "num_devices"

register_meta_struct    "clCreateImage", "image_format", "cl_image_format"
register_meta_struct    "clCreateImage", "image_desc", "cl_image_desc"

register_meta_parameter "clCreateProgramWithBuiltInKernels", InArray, "device_list", "num_devices"
register_meta_parameter "clCreateProgramWithBuiltInKernels", InString, "kernel_names"

register_meta_parameter "clCompileProgram", InArray, "device_list", "num_devices"
register_meta_parameter "clCompileProgram", InString, "options"
register_meta_parameter "clCompileProgram", InArray, "input_headers", "num_input_headers"
register_meta_parameter "clCompileProgram", InArray, "header_include_names", "num_input_headers"

register_meta_parameter "clLinkProgram", InArray, "device_list", "num_devices"
register_meta_parameter "clLinkProgram", InString, "options"
register_meta_parameter "clLinkProgram", InArray, "input_programs", "num_input_programs"

register_meta_parameter "clEnqueueFillBuffer", InArray, "pattern", "pattern_size"

register_meta_parameter "clEnqueueFillImage", InFixedArray, "origin", 3
register_meta_parameter "clEnqueueFillImage", InFixedArray, "region", 3

register_meta_parameter "clEnqueueMigrateMemObjects", InArray, "mem_objects", "num_mem_objects"

register_meta_parameter "clGetExtensionFunctionAddressForPlatform", InString, "func_name"

register_meta_parameter "clCreateFromEGLImageKHR", InNullArray, "properties"

register_meta_parameter "clEnqueueAcquireEGLObjectsKHR", InArray, "mem_objects", "num_objects"

register_meta_parameter "clEnqueueReleaseEGLObjectsKHR", InArray, "mem_objects", "num_objects"

register_meta_parameter "clCreateCommandQueueWithProperties", InNullArray, "properties"

register_meta_parameter "clCreatePipe", InNullArray, "properties"

register_meta_parameter "clEnqueueSVMFree", InArray, "svm_pointers", "num_svm_pointers"

register_meta_parameter "clEnqueueSVMMemFill", InArray, "pattern", "pattern_size"

register_meta_parameter "clCreateSamplerWithProperties", InNullArray, "sampler_properties"

register_meta_parameter "clSetKernelExecInfo", InArray, "param_value", "param_value_size"

register_meta_parameter "clEnqueueSVMMigrateMem", InArray, "svm_pointers", "num_svm_pointers"
register_meta_parameter "clEnqueueSVMMigrateMem", InArray, "sizes", "num_svm_pointers"

register_meta_parameter "clGetDeviceAndHostTimer", OutScalar, "device_timestamp"
register_meta_parameter "clGetDeviceAndHostTimer", OutScalar, "host_timestamp"

register_meta_parameter "clGetHostTimer", OutScalar, "host_timestamp"

register_meta_parameter "clGetKernelSubGroupInfo", InArray, "input_value", "input_value_size"

register_meta_parameter "clSetProgramSpecializationConstant", InArray, "spec_value", "spec_size"

$opencl_commands = funcs_e.collect { |func|
  Command::new(func)
}

$clReleaseEvent = $opencl_commands.find { |c| c.prototype.name == "clReleaseEvent" }
$clSetEventCallback = $opencl_commands.find { |c| c.prototype.name == "clSetEventCallback" }
$clGetEventProfilingInfo = $opencl_commands.find { |c| c.prototype.name == "clGetEventProfilingInfo" }
$clGetKernelInfo = $opencl_commands.find { |c| c.prototype.name == "clGetKernelInfo" }
$clGetMemObjectInfo = $opencl_commands.find { |c| c.prototype.name == "clGetMemObjectInfo" }
$clEnqueueReadBuffer = $opencl_commands.find { |c| c.prototype.name == "clEnqueueReadBuffer" }
$clGetCommandQueueInfo = $opencl_commands.find { |c| c.prototype.name == "clGetCommandQueueInfo" }
$clEnqueueBarrierWithWaitList = $opencl_commands.find { |c| c.prototype.name == "clEnqueueBarrierWithWaitList" }

create_sub_buffer = $opencl_commands.find { |c| c.prototype.name == "clCreateSubBuffer" }

buffer_create_info = InMetaParameter::new(create_sub_buffer, "buffer_create_info")
buffer_create_info.instance_variable_set(:@lttng_in_type, [:ctf_sequence_hex, :uint8_t, "buffer_create_info_vals", "buffer_create_info", "size_t", "buffer_create_info == NULL ? 0 : (buffer_create_type == CL_BUFFER_CREATE_TYPE_REGION ? sizeof(cl_buffer_region) : 0)"])

create_sub_buffer.meta_parameters.push buffer_create_info


$opencl_commands.each { |c|
  if c.prototype.name.match "clEnqueue"
    c.prologues.push <<EOF
  uint64_t _enqueue_counter = 0;
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
    pthread_mutex_lock(&opencl_obj_mutex);
    add_kernel(_retval);
    pthread_mutex_unlock(&opencl_obj_mutex);
  }
EOF

register_epilogue "clSetKernelArg", <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    pthread_mutex_lock(&opencl_obj_mutex);
    add_kernel_arg(kernel, arg_index, arg_size, arg_value);
    pthread_mutex_unlock(&opencl_obj_mutex);
  }
EOF

register_prologue "clEnqueueNDRangeKernel", <<EOF
  int _dump_release_events = 0;
  int _dump_release_event = 0;
  cl_event extra_event;
  if (do_dump && command_queue != NULL && kernel != NULL && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    cl_command_queue_properties properties;
    #{$clGetCommandQueueInfo.prototype.pointer_name}(command_queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &properties, NULL);
    pthread_mutex_lock(&opencl_obj_mutex);
    _dump_release_events = dump_kernel_args(command_queue, kernel, _enqueue_counter, properties, &num_events_in_wait_list, (cl_event **)&event_wait_list);
    pthread_mutex_unlock(&opencl_obj_mutex);
    if (properties | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE && event == NULL) {
      event = &extra_event;
      _dump_release_event = 1;
    }
  }
EOF

register_epilogue "clEnqueueNDRangeKernel", <<EOF
  if (do_dump && _dump_release_events) {
    for (int event_index = 0; event_index < num_events_in_wait_list; event_index++) {
      #{$clReleaseEvent.prototype.pointer_name}(event_wait_list[event_index]);
      free((void *)event_wait_list);
    }
  }
EOF

register_prologue "clCreateBuffer", <<EOF
  if (do_dump) {
    flags &= ~CL_MEM_HOST_WRITE_ONLY;
    flags &= ~CL_MEM_HOST_NO_ACCESS;
  }
EOF

register_epilogue "clCreateBuffer", <<EOF
  if (do_dump && _retval != NULL) {
    pthread_mutex_lock(&opencl_obj_mutex);
    add_buffer(_retval, size);
    pthread_mutex_unlock(&opencl_obj_mutex);
  }
EOF

register_prologue "clCreateCommandQueue", <<EOF
  if (tracepoint_enabled(#{provider}_profiling, event_profiling)) {
    properties |= CL_QUEUE_PROFILING_ENABLE;
  }
EOF

register_prologue "clCreateCommandQueueWithProperties", <<EOF
  cl_queue_properties *_profiling_properties = NULL;
  if (tracepoint_enabled(#{provider}_profiling, event_profiling)) {
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
  if (tracepoint_enabled(#{provider}_source, program_string) && strings != NULL) {
    int index;
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
      do_tracepoint(#{provider}_source, program_string, index, length, path);
    }
  }
EOF

register_prologue "clCreateProgramWithBinary", <<EOF
  if (tracepoint_enabled(#{provider}_source, program_binary) && binaries != NULL && lengths != NULL) {
    int index;
    for (index = 0; index < num_devices; index++) {
      char path[sizeof(BIN_SOURCE_TEMPLATE)];
      strncpy(path, BIN_SOURCE_TEMPLATE, sizeof(path));
      create_file_and_write(path, lengths[index], binaries[index]);
      do_tracepoint(#{provider}_source, program_binary, index, lengths[index], path);
    }
  }
EOF

register_prologue "clCreateProgramWithIL", <<EOF
  if (tracepoint_enabled(#{provider}_source, program_il) && il != NULL) {
    char path[sizeof(IL_SOURCE_TEMPLATE)];
    strncpy(path, IL_SOURCE_TEMPLATE, sizeof(path));
    create_file_and_write(path, length, il);
    do_tracepoint(#{provider}_source, program_il, length, path);
  }
EOF

$opencl_commands.each { |c|
  if c.event?
    if !c.returns_event?
      c.prologues.push <<EOF
  int _profile_release_event = 0;
  int _event_profiling = 0;
  cl_event profiling_event;
  if (tracepoint_enabled(#{provider}_profiling, event_profiling)) {
    if (event == NULL) {
      event = &profiling_event;
      _profile_release_event = 1;
    }
    _event_profiling = 1;
  }
EOF
      c.epilogues.push <<EOF
  if (_event_profiling) {
    int _set_retval = #{$clSetEventCallback.prototype.pointer_name}(*event, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling, _set_retval, *event);
    if(_profile_release_event) {
      #{$clReleaseEvent.prototype.pointer_name}(*event);
      event = NULL;
    }
  }
EOF
    else
      c.epilogues.push <<EOF
  if (tracepoint_enabled(#{provider}_profiling, event_profiling) ) {
    int _set_retval = #{$clSetEventCallback.prototype.pointer_name}(_retval, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling, _set_retval, _retval);
  }
EOF
    end
  end
}

register_epilogue "clEnqueueNDRangeKernel", <<EOF
  if (do_dump && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    if (_retval == CL_SUCCESS) {
      pthread_mutex_lock(&opencl_obj_mutex);
      cl_event ev = dump_kernel_buffers(command_queue, kernel, event);
      pthread_mutex_unlock(&opencl_obj_mutex);
      if (_dump_release_event) {
        #{$clReleaseEvent.prototype.pointer_name}(*event);
        event = NULL;
        if (ev != NULL) {
          #{$clReleaseEvent.prototype.pointer_name}(ev);
        }
      } else if ( ev != NULL ) {
        if (event != NULL) {
          if (*event != NULL)
            #{$clReleaseEvent.prototype.pointer_name}(*event);
          *event = ev;
        }
      }
    } else {
      if (_dump_release_event) {
        #{$clReleaseEvent.prototype.pointer_name}(*event);
        event = NULL;
      }
    }
  }
EOF

