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

  # `extra_objects` names are folded into the object list (hip seeds one type
  # the pointer-to-struct rule does not reach).
  def initialize(types:, structs:, unions:, enums:, extra_objects: [])
    @types = types
    @structs = structs
    @unions = unions
    @enums = enums

    @objects = find_objects(types, extra: extra_objects)
    @int_scalars = find_int_scalars(types)
    @enum_names, @bitfield_names, @struct_names = classify_ast_types(types, enums)
  end

  # The lookups the library generator does per typedef. They return nil for a
  # name the backend does not define: a typedef can name a struct that only
  # ever appears as an opaque forward declaration, which the generator skips
  # rather than emitting a layout for.
  def struct(name)
    @structs.find { |s| s.name == name }
  end

  def union(name)
    @unions.find { |u| u.name == name }
  end

  def enum(name)
    @enums.find { |e| e.name == name }
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

  # The "object" type names: typedefs of pointer-to-struct, plus any CustomType
  # aliasing a known OBJECT_TYPES name. `extra` names are inserted after the
  # pointer-to-struct seed and before the alias pass.
  def find_objects(all_types, extra: [])
    objects = all_types.filter_map do |t|
      t.name if t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Struct)
    end
    objects.concat(extra)
    all_types.each do |t|
      objects.push t.name if t.type.is_a?(YAMLCAst::CustomType) && OBJECT_TYPES.include?(t.type.name)
    end
    objects
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
