require 'nokogiri'
require 'yaml'

WINDOWS = /D3D|DX9/

VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$/

ABSENT_FUNCTIONS = /^clIcdGetPlatformIDsKHR$|^clCreateProgramWithILKHR$|^clTerminateContextKHR$|^clCreateCommandQueueWithPropertiesKHR$|^clEnqueueMigrateMemObjectEXT$/

#map = Hash::new { |h, k| h[k] = [] }

doc = Nokogiri::XML(open("cl.xml"))
funcs_e = doc.xpath("//commands/command").reject do |l|
  name = l.search("proto/name").text
  name.match(VENDOR_EXT) || name.match(ABSENT_FUNCTIONS) || name.match(WINDOWS)
end.collect


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
      if n.text.match("*")
        @__pointer == true
        break
      end
    } 
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

end

class Command < CLXML

  attr_reader :prototype
  attr_reader :parameters

  def initialize( command )
    super
    @prototype = Prototype::new( command.search("proto" ) )
    @parameters = command.search("param").collect { |p| Parameter::new(p) }
  end

  def decl
    "CL_API_ENTRY " + @prototype.decl + "(" + @parameters.collect(&:decl).join(", ") + ")"
  end

  def decl_pointer
    "CL_API_ENTRY " + @prototype.decl_pointer + "(" + @parameters.collect(&:decl_pointer).join(", ") + ")"
  end

end

$opencl_commands = funcs_e.collect { |func|
  Command::new(func)
}


