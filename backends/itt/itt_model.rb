require 'yaml'
require 'pp'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command.rb'
require_relative '../../utils/meta_parameters'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "ittResult"

$itt_api_yaml = YAML::load_file("itt_api.yaml")
$itt_api = YAMLCAst.from_yaml_ast($itt_api_yaml)

typedefs = $itt_api["typedefs"]
structs = $itt_api["structs"]

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS=/None/

$itt_meta_parameters = YAML::load_file(File.join(SRC_DIR, "itt_meta_parameters.yaml"))
$itt_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$itt_commands = $itt_api["functions"].collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ITT_POINTER_NAMES = $itt_commands.collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h


