require_relative 'command_index'
require_relative 'meta_parameter_spec'

# The per-backend facts the shared generators need but cannot derive from a
# function alone: what the traced return value is called, which functions
# initialize the API, the struct layouts to walk when a meta-parameter names a
# member, and how the API's typedefs classify.
#
# A Command belongs to exactly one backend, so it carries this rather than the
# shared generators reading it from whichever backend was required last.
BackendContext = Struct.new(:result_name, :init_functions, :struct_map, :type_classes,
                            keyword_init: true) do
  # The two derived fields always come from the same model, so a backend names
  # the model rather than restating where each one is read from.
  def self.for(api, result_name:, init_functions:)
    new(result_name: result_name, init_functions: init_functions,
        struct_map: api.struct_map, type_classes: api.type_classes)
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
#
# This lives here rather than on CommandIndex because it names Command, which
# opencl does not share.
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
    @meta_parameters = meta_parameters.collect do |type, args|
      type.new(self, *args)
    end
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
    @context.result_name
  end

  def type_classes
    @context.type_classes
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
    name + '_t'
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

    path = name.split('->')
    if path.length == 1
      res = parameters.find { |p| p.name == name }
      return res if res

      @tracepoint_parameters.find { |p| p.name == name }
    else
      param_name = path.shift
      res = parameters.find { |p| p.name == param_name }
      return nil unless res

      path.each do |n|
        res = @context.struct_map[res.type.type.name].find { |m| m.name == n }
        return nil unless res
      end
      res
    end
  end
end
