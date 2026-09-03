# What every AST-driven backend model starts from: the parsed-header stack it
# builds on, and where its YAML inputs live.
require_relative 'api_model'
require_relative 'yaml_ast_lttng'
require_relative 'LTTng'
require_relative 'command'
require_relative 'meta_parameters'

# The build system passes this; the fallback is for running a generator by hand
# from the directory its inputs live in.
SRC_DIR = ENV['SRC_DIR'] || '.'
