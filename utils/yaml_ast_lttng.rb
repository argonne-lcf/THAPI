require_relative 'yaml_ast'
require_relative 'LTTng'

FLOAT_SCALARS_MAP = { 'float' => 'uint32_t', 'double' => 'uint64_t' }

module YAMLCAst
  class Type
    def lttng_type(_type_classes)
      raise "Unsupported type #{self}!"
    end
  end

  class Void
    def lttng_type(_type_classes)
      nil
    end
  end

  class Int
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Float
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_float
      ev.type = name
      ev
    end
  end

  class Char
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Bool
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Struct
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :lttng_ust_field_fixed_length_blob
      ev.type = :uint8_t
      ev.length = "sizeof(struct #{name})"
      ev
    end

    def [](name)
      members.find { |m| m.name == name }
    end
  end

  class Union
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :lttng_ust_field_fixed_length_blob
      ev.type = :uint8_t
      ev.length = "sizeof(union #{name})"
      ev
    end
  end

  class Enum
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_integer
      ev.type = "enum #{name}"
      ev
    end
  end

  class Pointer
    def lttng_type(_type_classes)
      ev = LTTng::TracepointField.new
      ev.macro = :ctf_integer_hex
      ev.type = :uintptr_t
      ev.cast = 'uintptr_t'
      ev
    end
  end

  class Declaration
    def lttng_type(type_classes)
      r = type.lttng_type(type_classes)
      r.name = name
      r.expression = case type
                     when Struct, Union
                       "&#{name}"
                     when CustomType
                       type_classes.aggregate?(type.name) ? "&#{name}" : name
                     else
                       name
                     end
      r
    end
  end

  class CustomType
    def lttng_type(type_classes)
      ev = LTTng::TracepointField.new
      case type_classes.category_of(name)
      when :address
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = 'uintptr_t'
      when :hex_int
        ev.macro = :ctf_integer_hex
        ev.type = name
      when :integer
        ev.macro = :ctf_integer
        ev.type = name
      when :enum
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when :aggregate
        ev.macro = :lttng_ust_field_fixed_length_blob
        ev.type = :uint8_t
        ev.length = "sizeof(#{name})"
      else
        super
      end
      ev
    end
  end

  class Array
    def lttng_type(type_classes, length: nil, length_type: nil)
      ev = LTTng::TracepointField.new
      if length
        ev.length = length
      elsif self.length
        ev.length = self.length
      else
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = 'uintptr_t'
        return ev
      end
      if length_type
        lttng_arr_type = 'sequence'
        ev.length_type = length_type
      else
        lttng_arr_type = 'array'
      end
      case type
      when YAMLCAst::Pointer
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = :uintptr_t
      when YAMLCAst::Int
        ev.macro = :"ctf_#{lttng_arr_type}"
        ev.type = type.name
      when YAMLCAst::Float
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type_classes.array_category_of(type.name)
        when :address
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when :hex_int
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = type.name
        when :integer
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when :enum
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when :aggregate
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(#{type.name})"
            ev.length_type = 'size_t'
          end
          ev.macro = ev.length_type ? :lttng_ust_field_variable_length_blob : :lttng_ust_field_fixed_length_blob
        else
          super(type_classes)
        end
      else
        super(type_classes)
      end
      ev
    end
  end
end
