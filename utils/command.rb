require_relative 'LTTng'
require_relative 'command_index'
require_relative 'meta_parameter_spec'

# The per-backend facts the shared generators need but cannot derive from a
# function alone: what the traced return value is called, which functions
# initialize the API, the struct layouts to walk when a meta-parameter names a
# member, how the API's typedefs classify, and which events a call produces.
#
# A Command belongs to exactly one backend, so it carries this rather than the
# shared generators reading it from whichever backend was required last.
#
# `result_name` is one name for the whole API, or a callable naming the result
# of each command: OpenCL records an error code and a created object under
# different names.
#
# `pointer_type_suffix` is the suffix of the function-pointer typedefs. It
# defaults to `_t`, which the OpenCL headers already declare.
BackendContext = Struct.new(:result_name, :init_functions, :struct_map, :type_classes, :directions,
                            :pointer_type_suffix, keyword_init: true) do
  # The two derived fields always come from the same model, so a backend names
  # the model rather than restating where each one is read from.
  #
  # `directions` is the events one traced call produces. The tracepoint provider
  # and the babeltrace model both name events from it, so they cannot disagree.
  # A callback API passes [nil]: the runtime calls in once.
  def self.for(api, result_name:, init_functions:, directions: %i[start stop], **settings)
    new(result_name: result_name, init_functions: init_functions, directions: directions,
        struct_map: api.struct_map, type_classes: api.type_classes, **settings)
  end
end

# Build the command index a backend traces, and check its meta-parameter spec
# against it: three steps every AST backend takes in the same order.
#
# `groups` maps each LTTng provider to the functions it carries. `spec` is the
# backend's meta-parameter spec, whose rows are attached to the command they
# name and whose keys are then checked to name a real one.
#
# `select` narrows the functions traced, for a backend that wraps only some of
# what its headers declare: itt traces eight of the hundred-odd it parses.
def build_command_index(groups, context:, spec: Hash.new { [] }, select: ->(_func) { true })
  index = CommandIndex.new(groups.transform_values do |funcs|
    funcs.filter_map do |func|
      Command.new(func, context: context, meta_parameters: spec[func.name]) if select.call(func)
    end
  end)
  check_meta_parameters(spec, index)
  index
end

class Command
  attr_reader :tracepoint_parameters, :meta_parameters, :prologues, :epilogues, :function

  # `meta_parameters` is this function's rows from a meta-parameter spec, as
  # returned by load_meta_parameters: a list of [MetaParameter subclass, args].
  def initialize(function, context:, meta_parameters: [])
    @function = function
    @context = context
    @tracepoint_parameters = []
    @meta_parameters = meta_parameters.collect { |type, args| type.new(self, *args) }
    @prologues = []
    @epilogues = []
  end

  # An event carries every parameter computed by the time it fires: a :stop
  # parameter reads the result, so only the exit event -- never an entry, and
  # never a callback API's single undirected event -- can see one.
  def tracepoint_parameters_for(dir)
    dir == :stop ? @tracepoint_parameters : @tracepoint_parameters.select { |p| p.dir == :start }
  end

  # The C computing the parameters that belong to this tracepoint's block.
  def tracepoint_inits(dir)
    @tracepoint_parameters.select { |p| p.dir == dir }.collect(&:fill)
  end

  def result_name
    name = @context.result_name
    name.respond_to?(:call) ? name.call(self) : name
  end

  def type_classes
    @context.type_classes
  end

  def directions
    @context.directions
  end

  def name
    @function.name
  end

  def add_prologue(code)
    @prologues.push(code)
  end

  def add_epilogue(code)
    @epilogues.push(code)
  end

  def decl_pointer(name = pointer_name)
    YAMLCAst::Declaration.new(name: name, type: YAMLCAst::Pointer.new(type: @function.type), storage: 'typedef').to_s
  end

  def ffi_name
    name + '_ffi'
  end

  def decl_ffi_wrapper
    "void #{ffi_name}(ffi_cif *cif, #{type} *ffi_ret, void **args, #{pointer_type_name} #{pointer_name})"
  end

  def hidden_alias_name
    name + '_hid'
  end

  def decl_hidden_alias(name = hidden_alias_name)
    YAMLCAst::Declaration.new(name: name, type: @function.type,
                              storage: 'static').to_s + "__attribute__ ((alias (\"#{self.name}\")))"
  end

  def decl
    @function.to_s
  end

  def pointer_name
    name + '_ptr'
  end

  def pointer_type_name
    name + (@context.pointer_type_suffix || '_t')
  end

  def type
    @function.type.type
  end

  def parameters
    @function.type.params
  end

  def init?
    name.match(@context.init_functions)
  end

  def has_return_type?
    type && !type.is_a?(YAMLCAst::Void)
  end

  def [](name)
    # special case when querying the return value
    return YAMLCAst::Declaration.new(name: result_name, type: type) if name == :result

    param_name, *members = MemberPath.segments(name)
    res = parameters.find { |p| p.name == param_name }
    return @tracepoint_parameters.find { |p| p.name == name } if res.nil?

    members.each do |m|
      res = @context.struct_map[res.type.type.name].find { |member| member.name == m }
      return nil unless res
    end
    res
  end
end

# The libffi type of a parameter or of a command's return value, for the
# closures a backend builds around the extension functions it hands out.
def get_ffi_type(a)
  if a.type.is_a?(YAMLCAst::Void)
    'ffi_type_void'
  elsif a.type.is_a?(YAMLCAst::Pointer)
    'ffi_type_pointer'
  elsif FFI_TYPE_MAP["#{a.type}"]
    FFI_TYPE_MAP["#{a.type}"]
  else
    raise "Unsupported type: #{a.type}"
  end
end

# The epilogue of an extension-function lookup (ze's
# zeDriverGetExtensionFunctionAddress, opencl's clGetExtensionFunctionAddress):
# when it found one of `commands`, hand back a libffi closure that calls the
# tracer's `<command>_ffi` wrapper with the real function
# (utils/include/function_pointer_utils.h).
#
# `real` is the C holding the real pointer, `name_var` the looked-up name, and
# `found` the C that ends the lookup with `closure`, what to hand back.
def ffi_closure_epilogue(commands, condition:, provider:, name_var:, real:, found:)
  blocks = commands.map do |c|
    types = c.parameters.to_a.map { |a| "&#{get_ffi_type(a)}" }
    <<EOF
    if (tracepoint_enabled(#{provider}, #{c.name}_#{START}) && strcmp(#{name_var}, "#{c.name}") == 0) {
      static ffi_type *types[] = { #{(types + ['NULL']).join(', ')} };
      void *closure = thapi_closure_for(#{real}, (thapi_closure_wrapper)#{c.name}_ffi, &#{get_ffi_type(c)}, #{types.size}, types);
      if (closure != NULL) {
        #{found.strip.gsub("\n", "\n        ")}
      }
    }
EOF
  end
  "  if (#{condition}) {\n#{blocks.join("    else\n")}  }\n"
end
