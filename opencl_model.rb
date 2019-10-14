require 'nokogiri'
require 'yaml'

WINDOWS = /D3D|DX9/

VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$/

ABSENT_FUNCTIONS = /^clIcdGetPlatformIDsKHR$|^clCreateProgramWithILKHR$|^clTerminateContextKHR$|^clCreateCommandQueueWithPropertiesKHR$|^clEnqueueMigrateMemObjectEXT$/

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

CL_OBJECTS = ["cl_platform_id", "cl_device_id", "cl_context", "cl_command_queue", "cl_mem", "cl_program", "cl_kernel", "cl_event", "cl_sampler"]

CL_EXT_OBJECTS = ["CLeglImageKHR", "CLeglDisplayKHR", "CLeglSyncKHR"]

CL_INT_SCALARS = ["intptr_t", "size_t", "cl_int", "cl_uint", "cl_long", "cl_ulong", "cl_short", "cl_ushort", "cl_char", "cl_uchar"]
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

CL_OBJECTS_FORMAT = {
  "cl_platform_id" => "%p",
  "cl_device_id" => "%p",
  "cl_context" => "%p",
  "cl_command_queue" => "%p",
  "cl_mem" => "%p",
  "cl_program" => "%p",
  "cl_kernel" => "%p",
  "cl_event" => "%p",
  "cl_sampler" => "%p"
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

class Parameter < CLXML

  attr_reader :type
  attr_reader :name

  def initialize(param)
    super
    @name = param.search("name").text
    @type = param.search("type").text
    @__callback = nil
  end

  def decl
    @__node.children.collect(&:text).join(" ").squeeze(" ")
  end

  def decl_pointer
    @__node.children.collect { |n| "#{n.name == "name" ? "" : n.text}" }.join(" ").squeeze(" ")
  end

  def callback?
    @__callback if !@__callback.nil?
    @__callback = false
    @__node.children.collect { |n| @__callback = true if n.text.match("CL_CALLBACK") }
    @__callback
  end

  def pointer?
    @__pointer if !@__pointer.nil?
    @__pointer = callback?
    return @__pointer if @__pointer
    @__node.children.collect { |n|
      break if n.name == "name"
      if n.text.match("\\*")
        @__pointer = true
        break
      end
    }
    @__pointer
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
  def initialize(name, type)
    @name = name
    @type = type
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

class OutArray < OutMetaParameter
  def initialize(name, sname, type)
    @name = name
    @sname = sname
    @type = type
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_out_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_INT_SCALARS
      @lttng_out_type = [:ctf_sequence, type, name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_FLOAT_SCALARS
      @lttng_out_type = [:ctf_sequence_hex, CL_FLOAT_SCALARS_MAP[type], name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class InArray < InMetaParameter
  def initialize(name, sname, type)
    @name = name
    @sname = sname
    @type = type
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_in_type = [:ctf_sequence_hex, :intptr_t, name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_INT_SCALARS
      @lttng_in_type = [:ctf_sequence, type, name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    when *CL_FLOAT_SCALARS
      @lttng_in_type = [:ctf_sequence_hex, CL_FLOAT_SCALARS_MAP[type], name+"_vals", name, :size_t, "#{name} == NULL ? 0 : #{sname}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class AutoMetaParameter
  def self.create_if_match(command)
    nil
  end
end

class EventWaitList < AutoMetaParameter
  def self.create_if_match(command)
    el = command.parameters.select { |p| p.name == "event_wait_list" }.first
    if el
      return InArray::new("event_wait_list", "num_events_in_wait_list", "cl_event")
    end
    nil
  end
end

class ErrCodeRet < AutoMetaParameter
  def self.create_if_match(command)
    err = command.parameters.select { |p| p.name == "errcode_ret" }.first
    if err
      return OutScalar::new("errcode_ret", "cl_int")
    end
    nil
  end
end

class Event < AutoMetaParameter
  def self.create_if_match(command)
    ev = command.parameters.select { |p| p.name == "event" && p.pointer? }.first
    if ev
      return OutScalar::new("event", "cl_event")
    end
    nil
  end
end

def register_meta_parameter( mehtod, type, *args )
  META_PARAMETERS[mehtod].push type::new(*args)
end

AUTO_META_PARAMETERS = [EventWaitList, ErrCodeRet, Event]
META_PARAMETERS = Hash::new { |h, k| h[k] = [] }

class Command < CLXML

  attr_reader :prototype
  attr_reader :parameters
  attr_reader :meta_parameters

  def initialize( command )
    super
    @prototype = Prototype::new( command.search("proto" ) )
    @parameters = command.search("param").collect { |p| Parameter::new(p) }
    @meta_parameters = AUTO_META_PARAMETERS.collect { |klass| klass.create_if_match(self) }.compact
    @meta_parameters += META_PARAMETERS[@prototype.name]
  end

  def decl
    "CL_API_ENTRY " + @prototype.decl + "(" + @parameters.collect(&:decl).join(", ") + ")"
  end

  def decl_pointer
    "CL_API_ENTRY " + @prototype.decl_pointer + "(" + @parameters.collect(&:decl_pointer).join(", ") + ")"
  end

end


register_meta_parameter "clGetPlatformIDs", OutScalar, "num_platforms", "cl_uint"
register_meta_parameter "clGetPlatformIDs", OutArray, "platforms", "num_entries", "cl_platform_id"
register_meta_parameter "clEnqueueNDRangeKernel", InArray, "global_work_offset", "work_dim", "size_t"
register_meta_parameter "clEnqueueNDRangeKernel", InArray, "global_work_size", "work_dim", "size_t"
register_meta_parameter "clEnqueueNDRangeKernel", InArray, "local_work_size", "work_dim", "size_t"

$opencl_commands = funcs_e.collect { |func|
  Command::new(func)
}


