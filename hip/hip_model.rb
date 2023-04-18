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

funcs = $hip_api["functions"]
typedefs = $hip_api["typedefs"]
structs = $hip_api["structs"]

find_all_types(typedefs)
OBJECT_TYPES.push("hipGraphicsResource_t")
POINTER_TYPES.delete("hipGraphicsResource_t")
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS = /.*/

$hip_meta_parameters = YAML::load_file(File.join(SRC_DIR,"hip_meta_parameters.yaml"))
$hip_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$hip_commands = funcs.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

HIP_POINTER_NAMES = $hip_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h
