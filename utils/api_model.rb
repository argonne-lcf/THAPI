# One parsed api.yaml: the five lists it declares, plus the facts the
# generators derive from them.
#
# A backend whose headers span several namespaces (ze, cuda) adds its files
# together with `+`, because the derivations have to see every list at once: a
# zet typedef can name a ze struct, so classifying zet alone would get it
# wrong.
require_relative 'yaml_ast'

class ApiModel
  attr_reader :types, :structs, :unions, :enums, :functions, :hex_ints

  # A list the file does not declare reads back empty, so every model answers
  # all five questions.
  #
  # `hex_ints` names integer typedefs this API logs in hex: an address carried
  # in an integer reads as a decimal that means nothing to anyone.
  def self.load_file(path, hex_ints: [])
    lists = YAMLCAst.load_file(path)
    new(types: lists.fetch('typedefs', []), structs: lists.fetch('structs', []),
        unions: lists.fetch('unions', []), enums: lists.fetch('enums', []),
        functions: lists.fetch('functions', []), hex_ints: hex_ints)
  end

  # The five lists are what the YAML said, so they are frozen: a generator that
  # edited one would make its output depend on the order things ran in, and the
  # freeze turns that into a FrozenError at the site instead of a puzzle later.
  # It is shallow -- it guards the lists, not the AST nodes in them -- and `+`
  # builds new lists, so composing models still works.
  def initialize(types: [], structs: [], unions: [], enums: [], functions: [], hex_ints: [])
    @types = types.freeze
    @structs = structs.freeze
    @unions = unions.freeze
    @enums = enums.freeze
    @functions = functions.freeze
    @hex_ints = hex_ints.freeze
  end

  def +(other)
    ApiModel.new(types: types + other.types, structs: structs + other.structs,
                 unions: unions + other.unions, enums: enums + other.enums,
                 functions: functions + other.functions,
                 hex_ints: @hex_ints | other.hex_ints)
  end

  # How each typedef in this API is classified: objects, integers, pointers,
  # hex ints, enums, structs, unions.
  def type_classes
    @type_classes ||= find_all_types(@types, hex_ints: @hex_ints)
  end

  def int_scalars
    @int_scalars ||= find_int_scalars(@types, type_classes.integers).freeze
  end

  def struct_map
    @struct_map ||= find_struct_map(@types, @structs).freeze
  end

  # The struct a name refers to. Struct names are unique within an API, so this
  # answers in one lookup what a scan of `structs` answers in a linear pass.
  def struct_named(name)
    @structs_by_name ||= @structs.to_h { |s| [s.name, s] }.freeze
    @structs_by_name[name]
  end

  def enum_names
    classify
    @enum_names
  end

  def bitfield_names
    classify
    @bitfield_names
  end

  def struct_names
    classify
    @struct_names
  end

  # The definition a typedef refers to, or `type` itself when the layout was
  # declared inline and has no name of its own.
  #
  # A named target the API never defines is an opaque forward declaration, and
  # callers ask for that case with `opaque_ok`. Anything else raises: a missing
  # definition is otherwise a parse that dropped a type, which would silently
  # generate a library with a hole in it.
  def struct(type, opaque_ok: false)
    lookup(@structs, type, 'struct', opaque_ok)
  end

  def union(type, opaque_ok: false)
    lookup(@unions, type, 'union', opaque_ok)
  end

  def enum(type, opaque_ok: false)
    lookup(@enums, type, 'enum', opaque_ok)
  end

  def object?(name)
    type_classes.objects.include?(name)
  end

  # True when the generator has already emitted an FFI name for `name` and can
  # refer to it instead of inlining a layout.
  def typedef?(name)
    @types.any? { |t| t.type.respond_to?(:name) && t.type.name == name }
  end

  # The typedefs aliasing `name`, so the generator can chain a `typedef` line
  # for each further alias of a type it just emitted.
  def aliases_of(name)
    @types.select { |t| t.type.is_a?(YAMLCAst::CustomType) && t.type.name == name }
  end

  private

  def lookup(definitions, type, kind, opaque_ok)
    return type unless type.name

    definition = definitions.find { |d| d.name == type.name }
    return definition if definition
    raise "no definition for #{kind} #{type.name}" unless opaque_ok

    nil
  end

  # The typedef names this API declares, split by what they name. A typedef'd
  # enum whose underlying enum name ends in `flag_t` is a bitfield (OR-able
  # flags); every other enum is a plain enum. Each `_flag_t` bitfield
  # additionally aliases the `_flags_t` name the headers use for the OR'd
  # value.
  def classify
    return if @enum_names

    @enum_names = []
    @bitfield_names = []
    @struct_names = []
    @types.each do |t|
      case t.type
      when YAMLCAst::Enum
        e = @enums.find { |x| t.type.name == x.name }
        (e&.name&.end_with?('flag_t') ? @bitfield_names : @enum_names).push t.name
      when YAMLCAst::Struct
        @struct_names.push t.name
      end
    end
    @bitfield_names += @bitfield_names.select { |n| n.end_with?('_flag_t') }
                                      .map { |n| n.gsub('_flag_t', '_flags_t') }
    [@enum_names, @bitfield_names, @struct_names].each(&:freeze)
  end

  def find_int_scalars(all_types, integers)
    int_scalars = {}
    all_types.each do |t|
      int_scalars[t.name] = t.type.name if t.type.is_a?(YAMLCAst::CustomType) && integers.include?(t.type.name)
    end
    int_scalars
  end
end
