# The type facts the babeltrace-model generator needs to classify a field.
require_relative 'yaml_ast'

class TypeRegistry
  attr_reader :types_by_name, :enum_names, :bitfield_names, :struct_names,
              :integer_sizes, :integer_signed, :class_namer

  # The name lists come from ApiModel#classified; this only derives the
  # integer-size/-sign lookups (scalar widths plus a 32-bit signed entry per
  # enum) and the by-name type index.
  def self.from_ast(all_types:, all_enums:, enum_names:, bitfield_names:, struct_names:, class_namer:)
    integer_sizes = INT_SIZE_MAP.transform_values { |v| v * 8 }
    integer_signed = INT_SIGN_MAP.dup
    all_enums.each do |e|
      integer_sizes["enum #{e.name}"] = 32
      integer_signed["enum #{e.name}"] = true
    end

    new(types_by_name: all_types.map { |t| [t.name, t] }.to_h,
        enum_names: enum_names, bitfield_names: bitfield_names, struct_names: struct_names,
        integer_sizes: integer_sizes, integer_signed: integer_signed, class_namer: class_namer)
  end

  def initialize(types_by_name:, enum_names:, bitfield_names:, struct_names:,
                 integer_sizes:, integer_signed:, class_namer:)
    @types_by_name = types_by_name
    @enum_names = enum_names
    @bitfield_names = bitfield_names
    @struct_names = struct_names
    @integer_sizes = integer_sizes
    @integer_signed = integer_signed
    @class_namer = class_namer
  end

  def integer_size(t)
    return 64 if t.match(/\*/)
    return 64 if t.match(/\[.*\]/)

    r = integer_sizes[t]
    raise "unknown integer type #{t}" if r.nil?

    r
  end

  def integer_signed?(t)
    return false if t.match(/\*/)
    return false if t.match(/\[.*\]/)

    r = integer_signed[t]
    raise "unknown integer type #{t}" if r.nil?

    r
  end
end
