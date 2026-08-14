# Immutable value object holding one AST backend's parsed API: the typedefs,
# structs, unions and enums it was built from, plus the facts the library
# generator derives from them once.
#
# Every AST backend assembles the same four lists out of its *_api.yaml (ze and
# cuda concatenate theirs across namespaces) and derives the same three things,
# so the derivation lives here instead of being spelled out in each of the six
# gen_<x>_library_base.rb files.
#
# Backends publish theirs as the API constant, the same way they already
# publish FFI_STRUCT and FFI_UNION for the shared generator to read.
require_relative 'yaml_ast'

class ApiModel
  attr_reader :types, :structs, :unions, :enums,
              :objects, :int_scalars,
              :enum_names, :bitfield_names, :struct_names

  def initialize(types:, structs:, unions:, enums:)
    @types = types
    @structs = structs
    @unions = unions
    @enums = enums

    @objects = find_objects(types)
    @int_scalars = find_int_scalars(types)
    @enum_names, @bitfield_names, @struct_names = classify_ast_types(types, enums)
  end

  # The definition a typedef refers to. `type` is the typedef's target, so it
  # carries the layout itself when the struct or union was declared inline and
  # has no name of its own.
  #
  # A named target that the API does not define is an opaque forward
  # declaration: the headers name the struct but never give its layout, so
  # there is nothing to generate. Callers ask for that case explicitly with
  # `opaque_ok`, and anything else raises -- a missing definition is otherwise
  # a parse that dropped a type, which would silently generate a library with
  # a hole in it.
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
    @objects.include?(name)
  end

  # True when some typedef aliases `name`, i.e. the generator has already
  # emitted an FFI name for it and can refer to that instead of inlining a
  # layout.
  def typedef?(name)
    @types.any? { |t| t.type.respond_to?(:name) && t.type.name == name }
  end

  # The typedefs that alias `name`, so the generator can chain a `typedef` line
  # for every further alias of a type it just emitted.
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

  # A typedef'd enum whose underlying enum name ends in `flag_t` is a bitfield
  # (OR-able flags); every other enum is a plain enum. Each `_flag_t` bitfield
  # additionally aliases the `_flags_t` name the headers use for the OR'd
  # value. Backends with no `flag_t` enums get an empty bitfield list (the
  # `flag_t` test and `_flags_t` derivation are then no-ops), so the same rule
  # serves all backends.
  def classify_ast_types(all_types, all_enums)
    enum_names = []
    bitfield_names = []
    struct_names = []
    all_types.each do |t|
      case t.type
      when YAMLCAst::Enum
        e = all_enums.find { |x| t.type.name == x.name }
        (e&.name&.end_with?('flag_t') ? bitfield_names : enum_names).push t.name
      when YAMLCAst::Struct
        struct_names.push t.name
      end
    end
    bitfield_names += bitfield_names.select { |n| n.end_with?('_flag_t') }
                                    .map { |n| n.gsub('_flag_t', '_flags_t') }
    [enum_names, bitfield_names, struct_names]
  end

  # The "object" type names: the pointer-to-struct typedefs, plus every typedef
  # that aliases one of them, transitively.
  def find_objects(all_types)
    objects = all_types.filter_map do |t|
      t.name if object_typedef?(t, all_types)
    end
    transitive_closure(all_types, objects)
  end

  # Each typedef that aliases an integer type, mapped to that underlying type.
  def find_int_scalars(all_types)
    int_scalars = {}
    all_types.each do |t|
      int_scalars[t.name] = t.type.name if t.type.is_a?(YAMLCAst::CustomType) && INT_TYPES.include?(t.type.name)
    end
    int_scalars
  end
end
