# One parsed api.yaml: the five lists it declares, plus the facts the
# generators derive from them.
#
# A backend whose headers span several namespaces (ze, cuda) adds its files
# together with `+`, because the derivations have to see every list at once: a
# zet typedef can name a ze struct, so classifying zet alone would get it
# wrong.
require_relative 'yaml_ast'

class ApiModel
  ClassifiedNames = Struct.new(:enums, :bitfields, :structs, keyword_init: true)

  attr_reader :types, :structs, :unions, :enums, :functions

  # A list the file does not declare reads back empty, so every model answers
  # all five questions.
  def self.load_file(path)
    lists = YAMLCAst.load_file(path)
    new(types: lists.fetch('typedefs', []), structs: lists.fetch('structs', []),
        unions: lists.fetch('unions', []), enums: lists.fetch('enums', []),
        functions: lists.fetch('functions', []))
  end

  def initialize(types: [], structs: [], unions: [], enums: [], functions: [])
    @types = types
    @structs = structs
    @unions = unions
    @enums = enums
    @functions = functions
  end

  def +(other)
    ApiModel.new(types: types + other.types, structs: structs + other.structs,
                 unions: unions + other.unions, enums: enums + other.enums,
                 functions: functions + other.functions)
  end

  def type_classes
    @type_classes ||= find_all_types(@types)
  end

  def objects
    type_classes.objects
  end

  def int_scalars
    @int_scalars ||= find_int_scalars(@types, type_classes.integers)
  end

  def struct_map
    @struct_map ||= find_struct_map(@types, @structs)
  end

  def enum_names
    classified.enums
  end

  def bitfield_names
    classified.bitfields
  end

  def struct_names
    classified.structs
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
    objects.include?(name)
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
  def classified
    @classified ||= begin
      enums = []
      bitfields = []
      structs = []
      @types.each do |t|
        case t.type
        when YAMLCAst::Enum
          e = @enums.find { |x| t.type.name == x.name }
          (e&.name&.end_with?('flag_t') ? bitfields : enums).push t.name
        when YAMLCAst::Struct
          structs.push t.name
        end
      end
      bitfields += bitfields.select { |n| n.end_with?('_flag_t') }
                            .map { |n| n.gsub('_flag_t', '_flags_t') }
      ClassifiedNames.new(enums: enums, bitfields: bitfields, structs: structs)
    end
  end

  def find_int_scalars(all_types, integers)
    int_scalars = {}
    all_types.each do |t|
      int_scalars[t.name] = t.type.name if t.type.is_a?(YAMLCAst::CustomType) && integers.include?(t.type.name)
    end
    int_scalars
  end
end
