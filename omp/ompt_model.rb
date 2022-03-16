require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast'
require_relative '../utils/LTTng'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

START = "entry"
STOP = "exit"
SUFFIXES = { :start => START, :stop => STOP }

LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

provider = :lttng_ust_ompt

$ompt_api_yaml = YAML::load_file("ompt_api.yaml")

$ompt_api = YAMLCAst.from_yaml_ast($ompt_api_yaml)

ompt_funcs_e = $ompt_api["functions"]
ompt_types_e = $ompt_api["typedefs"]

all_types = ompt_types_e
all_structs = $ompt_api["structs"]

OMPT_OBJECTS = 
 
