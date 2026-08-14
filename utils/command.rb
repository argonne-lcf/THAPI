require_relative 'command_index'
require_relative 'meta_parameter_spec'

# The per-backend facts the shared generators need but cannot derive from a
# function alone: what the traced return value is called, which functions
# initialize the API, the struct layouts to walk when a meta-parameter names a
# member, and how the API's typedefs classify.
#
# These used to be ambient constants (RESULT_NAME, INIT_FUNCTIONS, STRUCT_MAP,
# TYPE_CLASSES) that shared code read from whichever backend happened to be
# required. Every reader already holds a Command, and a Command belongs to
# exactly one backend, so it carries this instead and the state is explicit.
BackendContext = Struct.new(:result_name, :init_functions, :struct_map, :type_classes,
                            keyword_init: true)

class Command
  attr_reader :tracepoint_parameters, :meta_parameters, :prologues, :epilogues, :function,
              :context

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

  # Shorthands for the context facts the generators reach for most.
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
