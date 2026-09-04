require 'nokogiri'
require 'yaml'
require_relative '../../utils/yaml_ast'
require_relative '../../utils/LTTng'
require_relative '../../utils/command_index'
require_relative '../../utils/meta_parameter_spec'

# The same suffixes utils/LTTng defines, keyed by the phase strings opencl's
# wrapper YAML and its generators use rather than by the AST backends' symbols.
CL_START = START
CL_STOP = STOP
CL_EVENT_SUFFIXES = { 'start' => CL_START, 'stop' => CL_STOP }.freeze

HOST_PROFILE = true

WINDOWS = /D3D|DX9/

VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$|OCLICD$/

ABSENT_FUNCTIONS = /^clIcdGetPlatformIDsKHR$|^clCreateProgramWithILKHR$|^clTerminateContextKHR$|^clCreateCommandQueueWithPropertiesKHR$|^clGetKernelSuggestedLocalWorkSizeKHR$|^clEnqueueMigrateMemObjectEXT$|^clGetLayerInfo$|^clInitLayer$/

EXTENSION_FUNCTIONS = /KHR$|EXT$|GL/

SUPPORTED_EXTENSION_FUNCTIONS = /#{yaml_load_file_cached(File.join(SRC_DIR, 'supported_extensions.yaml')).join('|')}/

INIT_FUNCTIONS = /clGetPlatformIDs|clGetPlatformInfo|clGetDeviceIDs|clCreateContext|clCreateContextFromType|clUnloadPlatformCompiler|clGetExtensionFunctionAddressForPlatform|clGetExtensionFunctionAddress|clGetGLContextInfoKHR/

# map = Hash::new { |h, k| h[k] = [] }

doc = Nokogiri::XML(open('cl.xml.patched'))
funcs_e = doc.xpath('//commands/command').reject do |l|
  name = l.search('proto/name').text
  name.match(VENDOR_EXT) || name.match(ABSENT_FUNCTIONS) || name.match(WINDOWS)
end.collect

ext_funcs_e = doc.xpath('//commands/command').select do |l|
  name = l.search('proto/name').text
  name.match(SUPPORTED_EXTENSION_FUNCTIONS)
end.collect

typedef_e = doc.xpath('//types/type').select do |l|
  l['category'] == 'define' && l.search('type').size > 0
end.collect

struct_e = doc.xpath('//types/type').select do |l|
  l['category'] == 'struct'
end.collect

CL_CONSTANTS = doc.xpath('//enums/enum').collect do |n|
  if n['value']
    [n['name'], n['value']]
  elsif n['bitpos']
    [n['name'], "(1 << #{n['bitpos']})"]
  end
end.to_h

CL_OBJECTS = %w[cl_platform_id cl_device_id cl_context cl_command_queue cl_mem cl_program cl_kernel
                cl_event cl_sampler]

CL_EXT_OBJECTS = %w[cl_GLsync CLeglImageKHR CLeglDisplayKHR CLeglSyncKHR cl_accelerator_intel]

CL_INT_SCALARS = ['unsigned int', 'int', 'uintptr_t', 'intptr_t', 'size_t', 'cl_int', 'cl_uint', 'cl_long', 'cl_ulong',
                  'cl_short', 'cl_ushort', 'cl_char', 'cl_uchar']
CL_FLOAT_SCALARS = %w[cl_half cl_float cl_double]
CL_FLOAT_SCALARS_MAP = { 'cl_half' => 'cl_ushort', 'cl_float' => 'cl_uint', 'cl_double' => 'cl_ulong' }
CL_BASE_TYPES = CL_INT_SCALARS + CL_FLOAT_SCALARS

CL_TYPE_MAP = typedef_e.collect do |l|
  [l.search('name').text, l.search('type').text]
end.to_h
(CL_BASE_TYPES + CL_EXT_OBJECTS + CL_OBJECTS).each do |t|
  CL_TYPE_MAP.delete(t)
end

err = false
CL_TYPE_MAP.transform_values! do |v|
  counter = 0
  until CL_BASE_TYPES.include?(v) || counter > 10
    counter += 1
    v = CL_TYPE_MAP[v]
  end
  err = true if counter > 10
  v
end
if err
  CL_TYPE_MAP.each do |k, v|
    warn "#{k}" unless v
  end
  raise 'Failed to achieve transitive closure!'
end

FFI_BASE_TYPES = %w[ffi_type_int ffi_type_uint ffi_type_uint8 ffi_type_sint8 ffi_type_uint16
                    ffi_type_sint16 ffi_type_uint32 ffi_type_sint32 ffi_type_uint64 ffi_type_sint64 ffi_type_float ffi_type_double ffi_type_void ffi_type_pointer]
FFI_TYPE_MAP = {
  'int' => 'ffi_type_int',
  'unsigned int' => 'ffi_type_uint',
  'uint8_t' => 'ffi_type_uint8',
  'int8_t' => 'ffi_type_sint8',
  'uint16_t' => 'ffi_type_uint16',
  'int16_t' => 'ffi_type_sint16',
  'uint32_t' => 'ffi_type_uint32',
  'int32_t' => 'ffi_type_sint32',
  'uint64_t' => 'ffi_type_uint64',
  'int64_t' => 'ffi_type_sint64',
  'float' => 'ffi_type_float',
  'double' => 'ffi_type_double',
  'intptr_t' => 'ffi_type_pointer',
  'uintptr_t' => 'ffi_type_pointer',
  'size_t' => 'ffi_type_pointer',
  'cl_double' => 'double',
  'cl_float' => 'float',
  'cl_char' => 'int8_t',
  'cl_uchar' => 'uint8_t',
  'cl_short' => 'int16_t',
  'cl_ushort' => 'uint16_t',
  'cl_int' => 'int32_t',
  'cl_uint' => 'uint32_t',
  'cl_long' => 'int64_t',
  'cl_ulong' => 'uint64_t',
  'cl_half' => 'uint8_t',
}

FFI_TYPE_MAP.merge! CL_TYPE_MAP

FFI_TYPE_MAP.transform_values! do |v|
  until FFI_BASE_TYPES.include? v
    ov = v
    v = FFI_TYPE_MAP[v]
    warn ov unless v
    warn FFI_BASE_TYPES unless v
    exit unless v
  end
  v
end

class CLXML
  attr_reader :__node

  def initialize(node)
    @__node = node
  end

  def inspect
    str = "#<#{self.class}:#{(object_id << 1).to_s(16)} "
    str << instance_variables.reject { |v|
      v == :@__node
    }.collect { |v| "#{v}=#{instance_variable_get(v).inspect}" }.join(', ')
    str << '>'
    str
  end
end

class Require < CLXML
  attr_reader :comment

  def initialize(node)
    super
    @comment = node['comment']
  end

  def bitfield?
    @comment.match('bitfield')
  end

  def enums
    @__node.search('enum').collect { |e| e['name'] }
  end
end

CL_REQUIRES = (doc.xpath('//feature/require').to_a + doc.xpath('//extensions/extension/require').to_a).collect do |r|
  Require.new(r)
end

class Declaration < CLXML
  attr_reader :type, :name

  def initialize(param)
    super
    @name = param.search('name').text
    @type = param.search('type').text
    @type += '*' if decl.match?(/\*\*/)
    @type += '*' if decl.match?(/\[\]/)
    @__callback = nil
  end

  def decl
    @__node.children.collect(&:text).join(' ').squeeze(' ')
  end

  def decl_pointer
    @__node.children.collect { |n| "#{n.text unless n.name == 'name'}" }.join(' ').squeeze(' ').strip
  end

  def pointer?
    return @__pointer unless @__pointer.nil?

    @__pointer = false
    @__node.children.collect do |n|
      break if n.name == 'name'

      if n.text.match('\\*')
        @__pointer = true
        break
      end
    end
    @__pointer
  end
end

class Member < Declaration
  def initialize(_command, member, prefix, dir = 'start')
    super(member)
    name = "#{prefix}#{MEMBER_SEPARATOR}#{@name}"
    expr = "#{prefix} != NULL ? #{prefix}->#{@name} : 0"
    @dir = dir
    @lttng_type = ['ctf_integer_hex', 'uintptr_t', name, "(uintptr_t)(#{expr})"] if pointer?
    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]
    case t
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      @lttng_type = ['ctf_integer_hex', 'uintptr_t', name, "(uintptr_t)(#{expr})"]
    when *CL_INT_SCALARS
      @lttng_type = ['ctf_integer', t, name, expr]
    when *CL_FLOAT_SCALARS
      @lttng_type = ['ctf_float', t, name, expr]
    end
  end

  def lttng_in_type
    @dir == 'start' ? @lttng_type : nil
  end

  def lttng_out_type
    @dir == 'start' ? nil : @lttng_type
  end
end

CL_STRUCT_MAP = struct_e.collect do |s|
  members = s.search('member')
  [s['name'], members]
end.to_h

CL_STRUCTS = CL_STRUCT_MAP.keys

class Parameter < Declaration
  def initialize(param)
    super
    @__callback = nil
  end

  def callback?
    return @__callback unless @__callback.nil?

    @__callback = false
    @__node.children.collect { |n| @__callback = true if n.text.match('CL_CALLBACK') }
    @__callback
  end

  def pointer?
    return true if callback?

    super
  end

  def lttng_in_type
    return ['ctf_integer_hex', 'uintptr_t', @name, "(uintptr_t)#{@name}"] if pointer?

    t = @type
    t = CL_TYPE_MAP[@type] if CL_TYPE_MAP[@type]

    case t
    when *CL_OBJECTS, *CL_EXT_OBJECTS
      return ['ctf_integer_hex', 'uintptr_t', @name, "(uintptr_t)#{@name}"]
    when *CL_INT_SCALARS
      return ['ctf_integer', t, @name, @name]
    when *CL_FLOAT_SCALARS
      return ['ctf_float', t, @name, @name]
    end

    nil
  end

  def void?
    decl.strip == 'void'
  end

  def lttng_out_type
    nil
  end

  def ffi_type
    return 'ffi_type_pointer' if pointer? || CL_OBJECTS.include?(type) || CL_EXT_OBJECTS.include?(type)
    return 'ffi_type_void' if void?

    FFI_TYPE_MAP[type]
  end
end

class Prototype < CLXML
  attr_reader :return_type, :name

  def has_return_type?
    return_type != 'void'
  end

  def ffi_return_type
    return 'ffi_type_void' unless has_return_type?
    if return_type.match(/\*/) || CL_OBJECTS.include?(return_type) || CL_EXT_OBJECTS.include?(return_type)
      return 'ffi_type_pointer'
    end

    FFI_TYPE_MAP[return_type]
  end

  def initialize(proto)
    super
    @name = proto.search('name').text
    @return_type = @__node.children.reject { |c| c.name == 'name' }.collect(&:text).join(' ').squeeze(' ').strip
  end

  def decl
    @__node.children.collect { |n| "#{'CL_API_CALL ' if n.name == 'name'}#{n.text}" }.join(' ').squeeze(' ')
  end

  def decl_pointer(type: false)
    @__node.children.collect do |n|
      "#{if n.name == 'name'
           "(CL_API_CALL *#{type ? pointer_type_name : pointer_name})"
         else
           n.text
         end}"
    end.join(' ').squeeze(' ')
  end

  def pointer_name
    @name + '_ptr'
  end

  def ffi_function_name
    @name + '_ffi'
  end

  def pointer_type_name
    @name + '_t'
  end

  def lttng_return_type
    return ['ctf_integer_hex', 'uintptr_t', '_retval', '(uintptr_t)_retval'] if @return_type.match('\\*')

    case @return_type
    when 'cl_int'
      return %w[ctf_integer cl_int errcode_ret_val _retval]

    when *CL_OBJECTS
      return ['ctf_integer_hex', 'uintptr_t', @return_type.gsub(/^cl_/, ''), '(uintptr_t)_retval']
    when *CL_EXT_OBJECTS
      return ['ctf_integer_hex', 'uintptr_t', @return_type.gsub(/^CL/, '').gsub(/KHR$/, ''), '(uintptr_t)_retval']
    when 'void*'
      return ['ctf_integer_hex', 'uintptr_t', 'ret_ptr', '(uintptr_t)_retval']
    end
    nil
  end
end

class MetaParameter
  def initialize(command, name, nocheck: false) # rubocop:disable Lint/UnusedMethodArgument
    @command = command
    @name = name
  end

  def lttng_array_type_broker(type, name, size, stype = nil)
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    if stype
      stype = CL_TYPE_MAP[stype] if CL_TYPE_MAP[stype]
      lttng_arr_type = 'sequence'
      lttng_args = [stype, "#{name} == NULL ? 0 : #{size}"]
    else
      lttng_arr_type = 'array'
      lttng_args = [size]
    end
    expr = name
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS, /\*/
      lttng_type = ["ctf_#{lttng_arr_type}_hex", 'uintptr_t']
    when *CL_INT_SCALARS
      lttng_type = ["ctf_#{lttng_arr_type}", type]
    when *CL_FLOAT_SCALARS
      lttng_type = ["ctf_#{lttng_arr_type}_hex", CL_FLOAT_SCALARS_MAP[type]]
    when *CL_STRUCTS, 'void'
      lttng_type = ["ctf_#{lttng_arr_type}_text", 'uint8_t']
    else
      raise "Unknown Type: #{type.inspect} for #{name} in #{@command.prototype.name}!"
    end
    lttng_type += [name + '_vals', expr]
    lttng_type + lttng_args
  end

  def lttng_in_type
    nil
  end

  def lttng_out_type
    nil
  end
end

class OutMetaParameter < MetaParameter
  attr_reader :lttng_out_type
end

class InMetaParameter < MetaParameter
  attr_reader :lttng_in_type
end

class InScalar < InMetaParameter
  def initialize(command, name, nocheck: false)
    super
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]

    type = command[name].type.gsub('*', '')
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS, 'void'
      @lttng_in_type = ['ctf_integer_hex', 'uintptr_t', name + '_val',
                        nocheck ? "(uintptr_t)(*#{name})" : "(uintptr_t)(#{name} == NULL ? 0 : *#{name})"]
    when *CL_INT_SCALARS
      @lttng_in_type = ['ctf_integer', type, name + '_val', nocheck ? "*#{name}" : "#{name} == NULL ? 0 : *#{name}"]
    when *CL_FLOAT_SCALARS
      @lttng_in_type = ['ctf_float', type, name + '_val', nocheck ? "*#{name}" : "#{name} == NULL ? 0 : *#{name}"]
    when *CL_STRUCTS
      @lttng_in_type = ['ctf_sequence_text', 'uint8_t', name + '_val', "(uint8_t *)#{name}", 'size_t',
                        "#{name} == NULL ? 0 : sizeof(#{type})"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class OutScalar < OutMetaParameter
  def initialize(command, name, nocheck: false)
    super
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]

    type = command[name].type.gsub('*', '')
    type = CL_TYPE_MAP[type] if CL_TYPE_MAP[type]
    case type
    when *CL_OBJECTS, *CL_EXT_OBJECTS, 'void'
      @lttng_out_type = ['ctf_integer_hex', 'uintptr_t', name + '_val',
                         nocheck ? "(uintptr_t)(*#{name})" : "(uintptr_t)(#{name} == NULL ? 0 : *#{name})"]
    when *CL_INT_SCALARS
      @lttng_out_type = ['ctf_integer', type, name + '_val', nocheck ? "*#{name}" : "#{name} == NULL ? 0 : *#{name}"]
    when *CL_FLOAT_SCALARS
      @lttng_out_type = ['ctf_float', type, name + '_val', nocheck ? "*#{name}" : "#{name} == NULL ? 0 : *#{name}"]
    else
      raise "Unknown Type: #{type.inspect}!"
    end
  end
end

class InFixedArray < InMetaParameter
  def initialize(command, name, count)
    super(command, name)
    raise "Couldn't find variable #{name} for #{command.prototype.name}!" unless command[name]

    type = command[name].type
    @lttng_in_type = lttng_array_type_broker(type, name, count)
  end
end

class OutArray < OutMetaParameter
  def initialize(command, name, sname = 'num_entries')
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
  def initialize(command, name, sname = 'num_entries')
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
    command.tracepoint_parameters.push TracepointParameter.new(sname, 'size_t', <<EOF)
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

# NULL terminated Key Value pairs
class InNullArray < InArray
  def initialize(command, name)
    sname = "_#{name}_size"
    command.tracepoint_parameters.push TracepointParameter.new(sname, 'size_t', <<EOF)
  #{sname} = 0;
  if(#{name} != NULL) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

class InString < InMetaParameter
  def initialize(command, name)
    super
    @lttng_in_type = ['ctf_string', name + '_val', name]
  end
end

class AutoMetaParameter
  def self.create_if_match(_command)
    nil
  end
end

class EventWaitList < AutoMetaParameter
  def self.create_if_match(command)
    el = command.parameters.find { |p| p.name == 'event_wait_list' }
    return InArray.new(command, 'event_wait_list', 'num_events_in_wait_list') if el

    nil
  end
end

class AutoOutScalar
  def self.create(name, nocheck: false)
    str = <<EOF
    Class::new(AutoMetaParameter) do
      def self.create_if_match(command)
        par = command.parameters.find { |p| p.name == "#{name}" && p.pointer? }
        if par
          return OutScalar::new(command, "#{name}", nocheck: #{nocheck})
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
    return nil if command.prototype.name == 'clSetKernelExecInfo'

    pv = command.parameters.find { |p| p.name == 'param_value' }
    return OutArray.new(command, 'param_value', 'param_value_size') if pv

    nil
  end
end

class TracepointParameter
  attr_reader :name, :type, :init

  def initialize(name, type, init)
    @name = name
    @type = type
    @init = init
  end
end

ErrCodeRet = AutoOutScalar.create('errcode_ret', nocheck: true)

ParamValueSizeRet = AutoOutScalar.create('param_value_size_ret', nocheck: true)

Event = AutoOutScalar.create('event')

AUTO_META_PARAMETERS = [EventWaitList, ErrCodeRet, ParamValueSizeRet, ParamValue, Event]

class Command < CLXML
  attr_reader :prototype, :parameters, :tracepoint_parameters, :meta_parameters, :prologues, :epilogues

  # `meta_parameters` is this function's rows from a meta-parameter spec, as
  # returned by load_meta_parameters: a list of [MetaParameter subclass, args].
  def initialize(command, meta_parameters: [])
    super(command)
    @prototype = Prototype.new(command.search('proto'))
    @parameters = command.search('param').collect { |p| Parameter.new(p) }
    @tracepoint_parameters = []
    @meta_parameters = AUTO_META_PARAMETERS.collect { |klass| klass.create_if_match(self) }.compact
    @meta_parameters += meta_parameters.collect do |type, args|
      type.new(self, *args)
    end
    @extension = @prototype.name.match(EXTENSION_FUNCTIONS)
    @init      = @prototype.name.match(INIT_FUNCTIONS)
    @prologues = []
    @epilogues = []
  end

  def name
    @prototype.name
  end

  def add_prologue(code)
    @prologues.push(code)
  end

  def add_epilogue(code)
    @epilogues.push(code)
  end

  def [](name)
    res = @parameters.find { |p| p.name == name }
    return res if res

    @tracepoint_parameters.find { |p| p.name == name }
  end

  def decl
    'CL_API_ENTRY ' + @prototype.decl + '(' + @parameters.collect(&:decl).join(', ') + ')'
  end

  def decl_pointer(type: false)
    'CL_API_ENTRY ' + @prototype.decl_pointer(type: type) + '(' + @parameters.collect(&:decl_pointer).join(', ') + ')'
  end

  def decl_ffi_wrapper
    "void #{@prototype.ffi_function_name}(ffi_cif *cif, #{@prototype.return_type} *ffi_ret, void** args, #{@prototype.pointer_type_name} #{@prototype.pointer_name})"
  end

  def event?
    returns_event? || @parameters.find { |p| p.name == 'event' && p.pointer? }
  end

  def returns_event?
    prototype.return_type == 'cl_event'
  end

  def extension?
    !!@extension
  end

  def init?
    !!@init
  end

  def void_parameters?
    @parameters.size == 1 && @parameters.first.void?
  end
end

meta_parameters = load_meta_parameters('opencl_meta_parameters.yaml')

# Both groups go to the one lttng_ust_opencl provider, so they are grouped by
# what actually separates them: an extension is reached through
# clGetExtensionFunctionAddress rather than dlsym, which several of the
# generators below need to tell apart.
OPENCL_COMMANDS = CommandIndex.new(
  { core: funcs_e, extension: ext_funcs_e }.transform_values do |funcs|
    funcs.collect { |func| Command.new(func, meta_parameters: meta_parameters[func.search('proto/name').text]) }
  end
)

check_meta_parameters(meta_parameters, OPENCL_COMMANDS)

# An extension is reached through libffi rather than dlsym, so its pointer
# keeps the header's own spelling instead of the upper-snake macro.
extension_commands = OPENCL_COMMANDS.groups[:extension]
OPENCL_POINTER_NAMES = OPENCL_COMMANDS.pointer_names(
  pointer_name_of: ->(c) { c.prototype.pointer_name }
) { |c, name| name if extension_commands.include?(c) }

OPENCL_COMMANDS.select do |c|
  c.parameters.find { |p| p.name == 'errcode_ret' && p.pointer? }
end.each do |c|
  c.add_prologue <<EOF
  cl_int _errcode_ret_force;
  if (!errcode_ret)
    errcode_ret = &_errcode_ret_force;
EOF
end

OPENCL_COMMANDS.select do |c|
  c.parameters.find { |p| p.name == 'param_value_size_ret' && p.pointer? }
end.each do |c|
  c.add_prologue <<EOF
  size_t _new_param_value_size;
  if (!param_value_size_ret)
    param_value_size_ret = &_new_param_value_size;
EOF
  c.add_epilogue <<EOF
  param_value_size = (param_value_size <= *param_value_size_ret ? param_value_size : *param_value_size_ret );
EOF
end

buffer_create_info = InMetaParameter.new(OPENCL_COMMANDS['clCreateSubBuffer'], 'buffer_create_info')
buffer_create_info.instance_variable_set(:@lttng_in_type,
                                         ['ctf_sequence_text', 'uint8_t', 'buffer_create_info_vals', 'buffer_create_info', 'size_t',
                                          'buffer_create_info == NULL ? 0 : (buffer_create_type == CL_BUFFER_CREATE_TYPE_REGION ? sizeof(cl_buffer_region) : 0)'])

OPENCL_COMMANDS['clCreateSubBuffer'].meta_parameters.push buffer_create_info

OPENCL_COMMANDS.each do |c|
  next unless c.prototype.name.match 'clEnqueue'

  c.add_prologue <<EOF
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

class ParamName < MetaParameter
  def initialize(c)
    super(c, 'param_name')
    raise "Couldn't find variable param_name for #{c.prototype.name}!" unless c['param_name']

    @type = c['param_name'].type.gsub('*', '')
  end

  def lttng_out_type
    ['ctf_integer_hex', @type, '_param_name', 'param_name']
  end
end

OPENCL_COMMANDS.each do |c|
  c.meta_parameters.push(ParamName.new(c)) if c.prototype.name.match(/clGet(\w*?)Info/) && c['param_name']
end

OPENCL_COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (do_dump && _retval != NULL) {
    add_kernel(_retval);
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clSetKernelArg', <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, arg_size, arg_value, 0);
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clSetKernelArgSVMPointer', <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, sizeof(arg_value), arg_value, 1);
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clSVMAlloc', <<EOF
  if (do_dump && _retval != NULL) {
    add_svmptr(_retval, size);
  }
EOF

OPENCL_COMMANDS.add_prologue 'clSVMFree', <<EOF
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
    #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clGetCommandQueueInfo']]}(command_queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &properties, NULL);
    _dump_release_events = dump_kernel_args(command_queue, kernel, _enqueue_counter, properties, &num_events_in_wait_list, (cl_event **)&event_wait_list);
    if ((properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) && event == NULL) {
      event = &extra_event;
      _dump_release_event = 1;
    }
  }
EOF
OPENCL_COMMANDS.add_prologue 'clEnqueueNDRangeKernel', str
OPENCL_COMMANDS.add_prologue 'clEnqueueNDRangeKernelINTEL', str

str = <<EOF
  if (do_dump && _dump_release_events) {
    for (cl_uint event_index = 0; event_index < num_events_in_wait_list; event_index++) {
      #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(event_wait_list[event_index]);
    }
    free((void *)event_wait_list);
  }
EOF
OPENCL_COMMANDS.add_epilogue 'clEnqueueNDRangeKernel', str
OPENCL_COMMANDS.add_epilogue 'clEnqueueNDRangeKernelINTEL', str

OPENCL_COMMANDS.add_prologue 'clCreateBuffer', <<EOF
  if (do_dump) {
    flags &= ~CL_MEM_HOST_WRITE_ONLY;
    flags &= ~CL_MEM_HOST_NO_ACCESS;
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clCreateBuffer', <<EOF
  if (do_dump && _retval != NULL) {
    add_buffer(_retval, size);
  }
EOF

OPENCL_COMMANDS.add_prologue 'clCreateCommandQueue', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    properties |= CL_QUEUE_PROFILING_ENABLE;
  }
EOF

OPENCL_COMMANDS.add_prologue 'clCreateCommandQueueWithProperties', <<EOF
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

OPENCL_COMMANDS.add_epilogue 'clCreateCommandQueueWithProperties', <<EOF
  if (_profiling_properties) free(_profiling_properties);
EOF

OPENCL_COMMANDS.add_prologue 'clCreateProgramWithSource', <<EOF
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

OPENCL_COMMANDS.add_prologue 'clCreateProgramWithBinary', <<EOF
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

OPENCL_COMMANDS.add_prologue 'clCreateProgramWithIL', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_il) && il != NULL) {
    char path[sizeof(IL_SOURCE_TEMPLATE)];
    strncpy(path, IL_SOURCE_TEMPLATE, sizeof(path));
    create_file_and_write(path, length, il);
    do_tracepoint(lttng_ust_opencl_source, program_il, length, path);
  }
EOF

OPENCL_COMMANDS.add_prologue 'clCreateProgramWithILKHR', <<EOF
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
OPENCL_COMMANDS.add_prologue 'clBuildProgram', str
OPENCL_COMMANDS.add_prologue 'clCompileProgram', str
OPENCL_COMMANDS.add_prologue 'clLinkProgram', <<EOF
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
OPENCL_COMMANDS.add_epilogue 'clBuildProgram', str
OPENCL_COMMANDS.add_epilogue 'clCompileProgram', str
OPENCL_COMMANDS.add_epilogue 'clLinkProgram', str

l = lambda { |func, name: 'pfn_notify', extra_conditions: nil|
  OPENCL_COMMANDS.add_prologue func, <<EOF
  struct #{func}_callback_payload *_payload = NULL;
  if ((tracepoint_enabled(lttng_ust_opencl, #{func}_callback_#{CL_START})#{if extra_conditions
                                                                             " || #{extra_conditions.join(' || ')}"
                                                                           end}) && #{name}) {
    _payload = (struct #{func}_callback_payload *)malloc(sizeof(struct #{func}_callback_payload));
    _payload->#{name} = #{name};
    _payload->user_data = user_data;
    user_data = (void *)_payload;
    #{name} = &#{func}_callback;
  }
EOF
}
program_conditions = ['tracepoint_enabled(lttng_ust_opencl_build, binaries)',
                      'tracepoint_enabled(lttng_ust_opencl_build, infos)']
l.call('clBuildProgram', extra_conditions: program_conditions)
l.call('clCompileProgram',
       extra_conditions: ['tracepoint_enabled(lttng_ust_opencl_build, objects)',
                          'tracepoint_enabled(lttng_ust_opencl_build, infos)'])
l.call('clLinkProgram', extra_conditions: program_conditions)
l.call('clCreateContext')
l.call('clCreateContextFromType')
l.call('clSetMemObjectDestructorCallback')
l.call('clSetProgramReleaseCallback')
l.call('clSetEventCallback')
l.call('clEnqueueSVMFree', name: 'pfn_free_func')

str = <<EOF
  if (_payload && _retval != CL_SUCCESS)
    free(_payload);
EOF
OPENCL_COMMANDS.add_epilogue 'clBuildProgram', str
OPENCL_COMMANDS.add_epilogue 'clCompileProgram', str
OPENCL_COMMANDS.add_epilogue 'clSetMemObjectDestructorCallback', str
OPENCL_COMMANDS.add_epilogue 'clSetProgramReleaseCallback', str
OPENCL_COMMANDS.add_epilogue 'clSetEventCallback', str
OPENCL_COMMANDS.add_epilogue 'clEnqueueSVMFree', str
str = <<EOF
  if (_payload && !_retval)
    free(_payload);
EOF
OPENCL_COMMANDS.add_epilogue 'clLinkProgram', str
OPENCL_COMMANDS.add_epilogue 'clCreateContext', str
OPENCL_COMMANDS.add_epilogue 'clCreateContextFromType', str

OPENCL_COMMANDS.add_epilogue 'clBuildProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && !pfn_notify) {
    dump_program_binaries(program);
  }
EOF
OPENCL_COMMANDS.add_epilogue 'clCompileProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, objects) && !pfn_notify) {
    dump_program_objects(program);
  }
EOF
OPENCL_COMMANDS.add_epilogue 'clLinkProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && _retval && !pfn_notify) {
    dump_program_binaries(_retval);
  }
EOF

str = <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && !pfn_notify) {
    dump_program_build_infos(program);
  }
EOF
OPENCL_COMMANDS.add_epilogue 'clBuildProgram', str
OPENCL_COMMANDS.add_epilogue 'clCompileProgram', str
OPENCL_COMMANDS.add_epilogue 'clLinkProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && _retval && !pfn_notify) {
    dump_program_build_infos(_retval);
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info)) {
    dump_kernel_info(_retval);
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval != NULL) {
    dump_kernel_arguments(program, _retval);
  }
EOF

OPENCL_COMMANDS.add_prologue 'clCreateKernelsInProgram', <<EOF
  cl_uint n_k = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_info(kernels[i]);
    }
  }
EOF

OPENCL_COMMANDS.add_prologue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_arguments(program, kernels[i]);
    }
  }
EOF

OPENCL_COMMANDS.add_prologue 'clGetDeviceIDs', <<EOF
  cl_uint n_e;
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_name) && !num_devices && devices) {
    num_devices = &n_e;
  }
EOF

OPENCL_COMMANDS.add_prologue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_timer) && !num_devices && devices) {
    num_devices = &n_e;
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_name) && _retval == CL_SUCCESS && devices) {
    for (cl_uint i = 0; i < *num_devices; i++) {
      dump_device_name(devices[i]);
    }
  }
EOF

OPENCL_COMMANDS.add_epilogue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_timer) && _retval == CL_SUCCESS && devices) {
    for (cl_uint i = 0; i < *num_devices; i++) {
      dump_device_timer(devices[i]);
    }
  }
EOF

str = OPENCL_COMMANDS.groups[:core].select { |c| c.extension? }.collect do |c|
  <<EOF
  if (strcmp(func_name, "#{c.prototype.name}") == 0) {
    tracepoint(lttng_ust_opencl, clGetExtensionFunctionAddressForPlatform_#{CL_STOP}, platform, func_name, (void *)(intptr_t)#{OPENCL_POINTER_NAMES[c]}#{if HOST_PROFILE
                                                                                                                                                           ', 0'
                                                                                                                                                         end});
    return (void *)(intptr_t)(&#{c.prototype.name});
  }
EOF
end.join(<<EOF)
  else
EOF

OPENCL_COMMANDS.add_prologue 'clGetExtensionFunctionAddressForPlatform', str

str = OPENCL_COMMANDS.groups[:core].select { |c| c.extension? }.collect do |c|
  <<EOF
  if (strcmp(func_name, "#{c.prototype.name}") == 0) {
    tracepoint(lttng_ust_opencl, clGetExtensionFunctionAddress_#{CL_STOP}, func_name, (void *)(intptr_t)#{OPENCL_POINTER_NAMES[c]}#{if HOST_PROFILE
                                                                                                                                      ', 0'
                                                                                                                                    end});
    return (void *)(intptr_t)(&#{c.prototype.name});
  }
EOF
end.join(<<EOF)
  else
EOF

OPENCL_COMMANDS.add_prologue 'clGetExtensionFunctionAddress', str

register_extension_callbacks = lambda { |ext_method|
  str = <<EOF
  if (_retval != NULL) {
EOF
  str << OPENCL_COMMANDS.groups[:extension].collect { |c|
    sstr = <<EOF
    if (tracepoint_enabled(lttng_ust_opencl, #{c.prototype.name}_#{CL_START}) && strcmp(func_name, "#{c.prototype.name}") == 0) {
      struct opencl_closure *closure = NULL;
      pthread_mutex_lock(&opencl_closures_mutex);
      HASH_FIND_PTR(opencl_closures, &_retval, closure);
      pthread_mutex_unlock(&opencl_closures_mutex);
      if (closure != NULL) {
        tracepoint(lttng_ust_opencl, #{ext_method}_#{CL_STOP},#{unless ext_method == 'clGetExtensionFunctionAddress'
                                                                  ' platform,'
                                                                end} func_name, _retval#{', _duration' if HOST_PROFILE});
        return closure->c_ptr;
      }
      closure = (struct opencl_closure *)malloc(sizeof(struct opencl_closure) + #{c.parameters.size} * sizeof(ffi_type *));
      if (closure != NULL) {
        closure->types = (ffi_type **)((intptr_t)closure + sizeof(struct opencl_closure));
        closure->closure = ffi_closure_alloc(sizeof(ffi_closure), &(closure->c_ptr));
        if (closure->closure != NULL) {
          closure->ptr = _retval;
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
              tracepoint(lttng_ust_opencl, #{ext_method}_#{CL_STOP},#{unless ext_method == 'clGetExtensionFunctionAddress'
                                                                        ' platform,'
                                                                      end} func_name, _retval#{if HOST_PROFILE
                                                                                                 ', _duration'
                                                                                               end});
              return closure->c_ptr;
            }
          }
          ffi_closure_free(closure->closure);
        }
        free(closure);
      }
    }
EOF
  }.join(<<EOF)
    else
EOF
  str << <<EOF
  }
EOF

  OPENCL_COMMANDS.add_epilogue ext_method, str
}

register_extension_callbacks.call('clGetExtensionFunctionAddress')
register_extension_callbacks.call('clGetExtensionFunctionAddressForPlatform')

# Create event profiling code
OPENCL_COMMANDS.each do |c|
  if c.event?
    if !c.returns_event?
      c.add_prologue <<EOF
  int _profile_release_event = 0;
  int _event_profiling = 0;
  cl_event _profiling_event = NULL;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    if (event == NULL) {
      event = &_profiling_event;
      _profile_release_event = 1;
    }
    _event_profiling = 1;
  }
EOF
      c.add_epilogue <<EOF
  if (_event_profiling) {
    if (_retval == CL_SUCCESS) {
      int _set_retval = #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clSetEventCallback']]}(*event, CL_COMPLETE, event_notify, NULL);
      do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, *event);
    }
    if(_profile_release_event) {
      #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(*event);
      event = NULL;
    }
  }
EOF
    elsif c.prototype.name != 'clCreateUserEvent'
      c.add_epilogue <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling) ) {
    int _set_retval = #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clSetEventCallback']]}(_retval, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, _retval);
  }
EOF
    end
  end
end

str = <<EOF
  if (do_dump && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    if (_retval == CL_SUCCESS) {
      cl_event ev = dump_kernel_buffers(command_queue, kernel, _enqueue_counter, event);
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(*event);
        event = NULL;
        if (ev != NULL) {
          #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(ev);
        }
      } else if ( ev != NULL ) {
        if (event != NULL) {
          if (*event != NULL)
            #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(*event);
          *event = ev;
        }
      }
    } else {
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[OPENCL_COMMANDS['clReleaseEvent']]}(*event);
        event = NULL;
      }
    }
  }
EOF
OPENCL_COMMANDS.add_epilogue 'clEnqueueNDRangeKernel', str
OPENCL_COMMANDS.add_epilogue 'clEnqueueNDRangeKernelINTEL', str
