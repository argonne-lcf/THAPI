require_relative './yaml_ast'

FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

module YAMLCAst

  class Type
    def lttng_type
      raise "Unsupported type #{self}!"
    end
  end

  class Void
    def lttng_type
      nil
    end
  end

  class Int
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Float
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_float
      ev.type = name
      ev
    end
  end

  class Char
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Bool
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Struct
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_sequence_text
      ev.type = :uint8_t
      ev.length_type = :size_t
      ev.length = "sizeof(struct #{name})"
      ev
    end

    def [](name)
      members.find { |m| m.name == name }
    end
  end

  class Union
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_sequence_text
      ev.type = :uint8_t
      ev.length_type = :size_t
      ev.length = "sizeof(union #{name})"
      ev
    end
  end

  class Enum
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = "enum #{name}"
      ev
    end
  end

  class Pointer
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer_hex
      ev.type = :uintptr_t
      ev.cast = "uintptr_t"
      ev
    end
  end

  class Declaration
    def lttng_type
      r = type.lttng_type
      r.name = name
      case type
      when Struct, Union
        r.expression = "&#{name}"
      when CustomType
        case type.name
        when *STRUCT_TYPES, *UNION_TYPES
          r.expression = "&#{name}"
        else
          r.expression = name
        end
      else
        r.expression = name
      end
      r
    end
  end

  class CustomType
    def lttng_type
      ev = LTTng::TracepointField::new
      case name
      when *OBJECT_TYPES, *POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when *HEX_INT_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = name
      when *INT_TYPES
        ev.macro = :ctf_integer
        ev.type = name
      when *ENUM_TYPES
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *STRUCT_TYPES, *UNION_TYPES
        ev.macro = :ctf_sequence_text
        ev.type = :uint8_t
        ev.length_type = :size_t
        ev.length = "sizeof(#{name})"
      else
        super
      end
      ev
    end
  end

  class Array
    def lttng_type(length: nil, length_type: nil)
      ev = LTTng::TracepointField::new
      if length
        ev.length = length
      elsif self.length
        ev.length = self.length
      else
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
        return ev
      end
      if length_type
        lttng_arr_type = "sequence"
        ev.length_type = length_type
      else
        lttng_arr_type = "array"
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
        ev.macro = :ctf_sequence_text
        ev.type = type.name
        ev.length_type = 'size_t' unless length_type
      when YAMLCAst::CustomType
        case type.name
        # Usually binary data or text
        when "uint8_t"
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(uint8_t)"
          end
        when *OBJECT_TYPES, *POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *HEX_INT_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = type.name
        when *INT_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *ENUM_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *STRUCT_TYPES, *UNION_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(#{type.name})"
          end
        else
          super
        end
      else
        super
      end
      ev
    end
  end

end
