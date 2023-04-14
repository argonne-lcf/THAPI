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

RESULT_NAME = "ompResult"

$ompt_api_yaml = YAML::load_file("ompt_api.yaml")
$ompt_api = YAMLCAst.from_yaml_ast($ompt_api_yaml)

ompt_funcs_e = $ompt_api["functions"]
ompt_types_e = $ompt_api["typedefs"]

all_types = ompt_types_e
all_structs = $ompt_api["structs"]
OBJECT_TYPES = ["ompt_device_t", "ompt_buffer_t","ompd_address_space_handle_t", "ompd_thread_handle_t", "ompd_parallel_handle_t",
                "ompd_task_handle_t","ompd_address_space_context_t", "ompd_thread_context_t"]
INT_TYPES = %w(int size_t uint8_t int64_t int32_t uint64_t char) + ["unsigned int"]
OMPT_FLOAT_SCALARS = ["double"]
OMPT_SCALARS = INT_TYPES + OMPT_FLOAT_SCALARS

all_types.each { |t|
  if  ( t.type.kind_of?(YAMLCAst::CustomType) || t.type.kind_of?(YAMLCAst::Int) ) && INT_TYPES.include?(t.type.name)
    INT_TYPES.push t.name
  end
}


ENUM_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
OMPT_CALLBACKS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Function) && t.name.match(/ompt_callback_.*_t/) }
                          .reject { |t| ["ompt_callback_buffer_complete_t","ompt_callback_buffer_request_t"].include?(t.name) }
                          .collect { |t| YAMLCAst::Declaration.new(name: t.name.gsub(/_t\z/,"")+"_func", type: t.type.type)}

STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of?(YAMLCAst::Struct) && !OBJECT_TYPES.include?(t.name) }.each { |t|
  if t.type.members
    STRUCT_MAP[t.name] = t.type.members
  else
    STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}

FFI_TYPE_MAP =  {
 "uint8_t" => "ffi_type_uint8",
 "uint64_t" => "ffi_type_uint64",
 "int64_t" => "ffi_type_sint64",
 "double" => "ffi_type_double",
 "uintptr_t" => "ffi_type_pointer",
 "size_t" => "ffi_type_pointer",
 "unsigned int" => "ffi_type_uint",
 "int" => "ffi_type_int",
 "char" => "ffi_type_char"
}

OBJECT_TYPES.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ENUM_TYPES.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

INIT_FUNCTIONS=/None/

HEX_INT_TYPES = []

$ompt_meta_parameters = YAML::load_file(File.join(SRC_DIR, "ompt_meta_parameters.yaml"))
$ompt_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$ompt_commands = OMPT_CALLBACKS.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

OMPT_POINTER_NAMES = $ompt_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h


