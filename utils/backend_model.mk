# The shared codegen stack every AST-driven backend model requires, as a
# prerequisite list. A backend's own <NAME>_MODEL adds its model script and its
# parsed headers to this.
#
# The list is here rather than repeated per backend so that adding a file to
# utils/backend_model.rb cannot leave six Makefiles out of date, each rebuilding
# from a stale generator until someone notices.
BACKEND_MODEL_DEPS = \
	$(top_srcdir)/utils/backend_model.rb \
	$(top_srcdir)/utils/api_model.rb \
	$(top_srcdir)/utils/yaml_ast.rb \
	$(top_srcdir)/utils/yaml_ast_lttng.rb \
	$(top_srcdir)/utils/meta_parameters.rb \
	$(top_srcdir)/utils/LTTng.rb \
	$(top_srcdir)/utils/command.rb \
	$(top_srcdir)/utils/command_index.rb \
	$(top_srcdir)/utils/meta_parameter_spec.rb
