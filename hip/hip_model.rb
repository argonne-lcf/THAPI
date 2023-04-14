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

RESULT_NAME = "hipResult"

$hip_api_yaml = YAML::load_file("hip_api.yaml")
$hip_api = YAMLCAst.from_yaml_ast($hip_api_yaml)

hip_funcs_e = $hip_api["functions"]

hip_types_e = $hip_api["typedefs"]

all_types = hip_types_e
all_structs = $hip_api["structs"]

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat( types.filter_map { |t|
      t.name if t.type.kind_of?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    } ).uniq!
    break if sz == arr.size
    sz = arr.size
  end
  arr
end

def transitive_closure_map(types, map)
  sz = map.size
  loop do
    types.select { |t|
      t.type.kind_of?(YAMLCAst::CustomType) && map.include?(t.type.name)
    }.each { |t| map[t.name] = map[t.type.name] }
    break if sz == map.size
    sz = map.size
  end
end

def find_types(types, cast_type, arr = nil)
  res = types.select { |t| t.type.kind_of? cast_type }.collect { |t| t.name }
  if arr
    arr.concat res
    res = arr
  end
  transitive_closure(types, res)
end

def find_types_map(types, cast_type, map)
  res = types.select { |t| t.type.kind_of? cast_type }.each { |t|
    map[t.name] = map[t.type.name]
  }
  transitive_closure_map(types, res)
end

OBJECT_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
transitive_closure(all_types, OBJECT_TYPES)
OBJECT_TYPES.push "hipGraphicsResource_t"

INT_TYPES = %w(size_t uintptr_t uint32_t uint64_t int short char)
INT_TYPES.concat [ "long long", "unsigned long long", "unsigned long long int", "unsigned int", "unsigned short", "unsigned char" ]
find_types(all_types, YAMLCAst::Int, INT_TYPES)

ENUM_TYPES = find_types(all_types, YAMLCAst::Enum)
STRUCT_TYPES = find_types(all_types, YAMLCAst::Struct)
UNION_TYPES = find_types(all_types, YAMLCAst::Union)
POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

POINTER_TYPES.delete("hipGraphicsResource_t")

STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    STRUCT_MAP[t.name] = t.type.members
  else
    mapped = all_structs.find { |str| str.name == t.type.name }
    STRUCT_MAP[t.name] = mapped.members if mapped
  end
}
transitive_closure_map(all_types, STRUCT_MAP)

INIT_FUNCTIONS = /.*/

FFI_TYPE_MAP =  {
  "unsigned char" => "ffi_type_uint8",
  "char" => "ffi_type_sint8",
  "unsigned short" => "ffi_type_uint16",
  "short" => "ffi_type_sint16",
  "unsigned int" => "ffi_type_uint32",
  "int" => "ffi_type_sint32",
  "unsigned long long" => "ffi_type_uint64",
  "unsigned long long int" => "ffi_type_uint64",
  "long long" => "ffi_type_sint64",
  "uint32_t" => "ffi_type_uint32",
  "uint64_t" => "ffi_type_uint64",
  "float" => "ffi_type_float",
  "double" => "ffi_type_double",
  "size_t" => "ffi_type_pointer",
}
transitive_closure_map(all_types, FFI_TYPE_MAP)

OBJECT_TYPES.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ENUM_TYPES.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

HEX_INT_TYPES = []

$hip_meta_parameters = YAML::load_file(File.join(SRC_DIR,"hip_meta_parameters.yaml"))
$hip_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$hip_commands = hip_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

HIP_POINTER_NAMES = $hip_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h
