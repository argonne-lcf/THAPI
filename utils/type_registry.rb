# Immutable value object holding the type facts the babeltrace-model generator
# needs to classify a field. Threaded explicitly instead of read from globals.
class TypeRegistry
  attr_reader :types_by_name, :enum_names, :bitfield_names, :struct_names,
              :integer_sizes, :integer_signed, :class_namer

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
