require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast_lttng'
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

OBJECT_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && OBJECT_TYPES.include?(t.type.name)
    OBJECT_TYPES.push t.name
  end
}

INT_TYPES = %w(size_t uint32_t uint64_t int short char cudaTextureObject_t cudaSurfaceObject_t)
INT_TYPES.concat [ "unsigned long long", "unsigned int", "unsigned short", "unsigned char" ]
CUDART_FLOAT_SCALARS = %w(float double)
CUDART_SCALARS = INT_TYPES + CUDART_FLOAT_SCALARS
ENUM_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

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

OBJECT_TYPES.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ENUM_TYPES.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

HEX_INT_TYPES = []

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
