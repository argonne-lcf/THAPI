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
# The settings after `directions` keep the C a backend generated before it
# moved to these generators byte-for-byte (opencl). Each one defaults to the
# shared spelling:
#
#   result_field  the field the return value is recorded in, from the command
#                 (default: result_name)
#   duration      every exit event also carries the call's `_duration`
#   null_guard    spells "the expression if these pointers are non-NULL, else
#                 the default": ->(checks, expr, default, cast:) { C }
#   xml_arg_spelling  tracepoint arguments spelled as the Khronos registry
#                 writes them: a callback as `void *`, `T x[]` as `T * *`
#   legacy_blobs  a pointed-to struct is cast to `(uint8_t *)`, and an array of
#                 structs or of void is a blob whose length is its element count
#   null_test     spells "this pointer is not NULL": ->(pointer) { C }
#   pointer_type_suffix  of the function-pointer typedefs (default `_t`; the
#                 OpenCL headers already declare `<function>_t`)
#   declarations  the C declaration of each function as written by hand rather
#                 than printed from the AST: {name => {'decl' =>, 'decl_pointer'
#                 => (with %s for the typedef name)}}
BackendContext = Struct.new(:result_name, :init_functions, :struct_map, :type_classes, :directions,
                            :result_field, :duration, :null_guard, :xml_arg_spelling, :legacy_blobs,
                            :null_test, :pointer_type_suffix, :declarations,
                            keyword_init: true) do
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
  # returned by load_meta_parameters: a list of [MetaParameter subclass, args, options].
  def initialize(function, context:, meta_parameters: [])
    @function = function
    @context = context
    @tracepoint_parameters = []
    @meta_parameters = meta_parameters.collect do |type, args, options|
      type.new(self, *args, **options.to_h)
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

  def result_field
    @context.result_field ? @context.result_field.call(self) : result_name
  end

  def duration?
    !!@context.duration
  end

  def xml_arg_spelling?
    !!@context.xml_arg_spelling
  end

  def legacy_blobs?
    !!@context.legacy_blobs
  end

  def null_test(pointer)
    @context.null_test ? @context.null_test.call(pointer) : pointer
  end

  def null_guard(checks, expr, default, cast: false)
    return @context.null_guard.call(checks, expr, default, cast: cast) if @context.null_guard

    "(#{checks.join(' && ')} ? #{expr} : #{default})"
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
    if (spelled = declaration('decl_pointer'))
      "typedef #{format(spelled, name)}"
    else
      YAMLCAst::Declaration.new(name: name, type: YAMLCAst::Pointer.new(type: @function.type), storage: 'typedef').to_s
    end
  end

  # How the backend spells this function's declaration, when it does.
  def declaration(key)
    @context.declarations&.dig(name, key)
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
    declaration('decl') || @function.to_s
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
# when it found one of `commands`, wrap the real function in a libffi closure
# that calls the tracer's `<command>_ffi` wrapper, and hand that back instead.
# Each driver returns its own function, so there is one closure per real
# pointer, kept in `<prefix>_closures`.
#
# `real` is the C holding the real pointer, `slot` its address (the hash key),
# `name_var` the looked-up name, and `found` the C that ends the lookup with
# the closure's `c_ptr`.
def ffi_closure_epilogue(commands, condition:, provider:, name_var:, prefix:, real:, slot:, found:)
  blocks = commands.map do |c|
    n = c.parameters.size
    types = c.parameters.each_with_index.map { |a, i| "\n          closure->types[#{i}] = &#{get_ffi_type(a)};" }
    <<EOF
    if (tracepoint_enabled(#{provider}, #{c.name}_#{START}) && strcmp(#{name_var}, "#{c.name}") == 0) {
      struct #{prefix}_closure *closure = NULL;
      pthread_mutex_lock(&#{prefix}_closures_mutex);
      HASH_FIND_PTR(#{prefix}_closures, #{slot}, closure);
      pthread_mutex_unlock(&#{prefix}_closures_mutex);
      if (closure != NULL) {
        #{found.strip.gsub("\n", "\n        ")}
      }
      closure = (struct #{prefix}_closure *)malloc(sizeof(struct #{prefix}_closure) + #{n} * sizeof(ffi_type *));
      if (closure != NULL) {
        closure->types = (ffi_type **)((intptr_t)closure + sizeof(struct #{prefix}_closure));
        closure->closure = ffi_closure_alloc(sizeof(ffi_closure), &(closure->c_ptr));
        if (closure->closure != NULL) {
          closure->ptr = #{real};#{types.join}
          if (ffi_prep_cif(&(closure->cif), FFI_DEFAULT_ABI, #{n}, &#{get_ffi_type(c)}, closure->types) == FFI_OK) {
            if (ffi_prep_closure_loc(closure->closure, &(closure->cif), (void (*)(ffi_cif *, void *, void **, void *))#{c.name}_ffi, #{real}, closure->c_ptr) == FFI_OK) {
              pthread_mutex_lock(&#{prefix}_closures_mutex);
              HASH_ADD_PTR(#{prefix}_closures, ptr, closure);
              pthread_mutex_unlock(&#{prefix}_closures_mutex);
              #{found.strip.gsub("\n", "\n              ")}
            }
          }
          ffi_closure_free(closure->closure);
        }
        free(closure);
      }
    }
EOF
  end
  "  if (#{condition}) {\n#{blocks.join("    else\n")}  }\n"
end
