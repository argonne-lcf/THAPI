require 'yaml'
require_relative 'command_index'

class Command
  attr_reader :tracepoint_parameters, :meta_parameters, :prologues, :epilogues, :function

  # `meta_parameters` is this function's rows from a meta-parameter spec, as
  # returned by load_meta_parameters: a list of [MetaParameter subclass, args].
  def initialize(function, meta_parameters: [])
    @function = function
    @tracepoint_parameters = []
    @meta_parameters = meta_parameters.collect do |type, args|
      type.new(self, *args)
    end
    @prologues = []
    @epilogues = []
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
    name.match(INIT_FUNCTIONS)
  end

  def has_return_type?
    type && !type.is_a?(YAMLCAst::Void)
  end

  def [](name)
    # special case when querying the return value
    return YAMLCAst::Declaration.new(name: "#{RESULT_NAME}", type: type) if name == :result

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
        res = STRUCT_MAP[res.type.type.name].find { |m| m.name == n }
        return nil unless res
      end
      res
    end
  end
end

# Read a backend's meta-parameter YAML (relative to SRC_DIR) and return the
# spec it describes: a Hash mapping each function name to a list of
# [MetaParameter subclass, args] rows, ready to hand to Command.new. Unlisted
# functions read back as [], so callers can `spec[name]` unconditionally.
#
# The file maps each function name to a list of [type, *args] rows under a
# top-level `meta_parameters` key. A backend with none of its own ships no
# file and calls Command.new without a spec, so that key must be a mapping: a
# missing, misspelled or empty one means a typo, which would otherwise yield
# an empty spec and look identical to having none.
#
# Pass several filenames to merge their specs (ze splits its rows per
# namespace, cuda across its two APIs). Each function must be declared in at
# most one of them: listing it twice would silently concatenate both sets of
# rows, so it raises instead.
def load_meta_parameters(*filenames)
  spec = Hash.new { [] }
  filenames.each do |filename|
    path = File.join(SRC_DIR, filename)
    content = YAML.load_file(path)
    entries = content['meta_parameters']
    raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

    rows = entries.transform_values do |list|
      list.collect { |type, *args| [Kernel.const_get(type), args] }
    end
    spec.merge!(rows) { |func, _, _| raise "#{func} is declared twice, second time in #{path}" }
  end
  spec
end
