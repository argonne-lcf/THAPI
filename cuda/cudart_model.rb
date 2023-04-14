require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast'
require_relative '../utils/LTTng'
require_relative '../utils/command.rb'
require_relative '../utils/meta_parameters'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "cudaResult"

$cudart_api_yaml = YAML::load_file("cudart_api.yaml")

$cudart_api = YAMLCAst.from_yaml_ast($cudart_api_yaml)

cudart_funcs_e = $cudart_api["functions"]

cudart_types_e = $cudart_api["typedefs"]

all_types = cudart_types_e
all_structs = $cudart_api["structs"]

CUDART_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && CUDART_OBJECTS.include?(t.type.name)
    CUDART_OBJECTS.push t.name
  end
}

CUDART_INT_SCALARS = %w(size_t uint32_t uint64_t int short char cudaTextureObject_t cudaSurfaceObject_t)
CUDART_INT_SCALARS.concat [ "unsigned long long", "unsigned int", "unsigned short", "unsigned char" ]
CUDART_FLOAT_SCALARS = %w(float double)
CUDART_SCALARS = CUDART_INT_SCALARS + CUDART_FLOAT_SCALARS
CUDART_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
CUDART_UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
CUDART_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    STRUCT_MAP[t.name] = t.type.members
  else
    STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}

FFI_TYPE_MAP =  {
 "uint8_t" => "ffi_type_uint8",
 "int8_t" => "ffi_type_sint8",
 "uint16_t" => "ffi_type_uint16",
 "int16_t" => "ffi_type_sint16",
 "uint32_t" => "ffi_type_uint32",
 "int32_t" => "ffi_type_sint32",
 "uint64_t" => "ffi_type_uint64",
 "int64_t" => "ffi_type_sint64",
 "float" => "ffi_type_float",
 "double" => "ffi_type_double",
 "uintptr_t" => "ffi_type_pointer",
 "size_t" => "ffi_type_pointer",
 "cudaTextureObject_t" => "ffi_type_uint64",
 "cudaSurfaceObject_t" => "ffi_type_uint64"
}

CUDART_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

CUDART_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

CUDART_FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

module YAMLCAst
  class Declaration
    def lttng_type
      r = type.lttng_type
      r.name = name
      case type
      when Struct, Union
        r.expression = "&#{name}"
      when CustomType
        case type.name
        when *STRUCT_TYPES, *CUDART_UNION_TYPES
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

  class Type
    def lttng_type
      raise "Unsupported type #{self}!"
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
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
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
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
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

  class CustomType
    def lttng_type
      ev = LTTng::TracepointField::new
      case name
      when *CUDART_OBJECTS, *CUDART_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when *CUDART_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *CUDART_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *STRUCT_TYPES, *CUDART_UNION_TYPES
        ev.macro = :ctf_array_text
        ev.type = :uint8_t
        ev.length = "sizeof(#{name})"
      else
        super
      end
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
        ev.type = CUDART_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *CUDART_OBJECTS, *CUDART_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *CUDART_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *CUDART_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *STRUCT_TYPES, *CUDART_UNION_TYPES
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

$cudart_meta_parameters = YAML::load_file(File.join(SRC_DIR,"cudart_meta_parameters.yaml"))
$cudart_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$cudart_commands = cudart_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

CUDART_POINTER_NAMES = $cudart_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h
