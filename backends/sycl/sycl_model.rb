require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command.rb'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "syclResult"

$sycl_api_yaml = YAML::load_file("sycl_api.yaml")
$sycl_api = YAMLCAst.from_yaml_ast($sycl_api_yaml)

typedefs = $sycl_api["typedefs"]
structs = $sycl_api["structs"]

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS=/None/

$sycl_commands = $sycl_api["functions"].filter_map { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

SYCL_POINTER_NAMES = $sycl_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h


