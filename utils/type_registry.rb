# The type facts the babeltrace-model generator needs to classify a field.
require_relative 'yaml_ast'

class TypeRegistry
  attr_reader :types_by_name, :enum_names, :bitfield_names, :struct_names, :class_namer

  # The name lists come from ApiModel#classified; this only derives the
  # integer-size/-sign lookups (the C scalar widths, plus a 32-bit signed entry
  # per enum) and the by-name type index.
  def initialize(all_types:, all_enums:, enum_names:, bitfield_names:, struct_names:, class_namer:)
    @types_by_name = all_types.to_h { |t| [t.name, t] }
    @enum_names = enum_names
    @bitfield_names = bitfield_names
    @struct_names = struct_names
    @class_namer = class_namer

    @integer_sizes = INT_SIZE_MAP.transform_values { |v| v * 8 }
    @integer_signed = INT_SIGN_MAP.dup
    all_enums.each do |e|
      @integer_sizes["enum #{e.name}"] = 32
      @integer_signed["enum #{e.name}"] = true
    end
  end

  # A pointer or an array is an address, whatever it points at.
  def integer_size(type)
    return 64 if type.match(/\*|\[.*\]/)

    @integer_sizes.fetch(type) { raise "unknown integer type #{type}" }
  end

  def integer_signed?(type)
    return false if type.match(/\*|\[.*\]/)

    @integer_signed.fetch(type) { raise "unknown integer type #{type}" }
  end
end
