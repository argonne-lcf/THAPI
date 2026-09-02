# What every AST-driven backend model starts from: the parsed-header stack it
# builds on, and where its YAML inputs live.
#
# A model requires this rather than restating the list, so the stack is
# described in one place and no backend can end up with a different subset of
# it by accident.
require 'yaml'
require 'set'

require_relative 'api_model'
require_relative 'yaml_ast_lttng'
require_relative 'LTTng'
require_relative 'command'
require_relative 'meta_parameters'

# The generators run from the build tree, so the directory holding a backend's
# own YAML inputs is passed in by the build system.
SRC_DIR = ENV['SRC_DIR'] || '.'
