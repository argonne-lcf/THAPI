require 'yaml'
require 'pp'

provider = :lttng_ust_ze


ze_api = YAML::load_file("ze_api.yaml")
zex_api = YAML::load_file("zex_api.yaml")
zet_api = YAML::load_file("zet_api.yaml")

ze_funcs_e = ze_api["functions"]
zex_funcs_e = zex_api["functions"]
zet_funcs_e = zet_api["functions"]

pp ze_funcs_e.collect { |f| f["name"] }
pp zex_funcs_e.collect { |f| f["name"] }
pp zet_funcs_e.collect { |f| f["name"] }

INIT_FUNCTIONS = /zeInit/
