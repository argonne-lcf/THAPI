if STRICT
  WERROR = -Werror
else
  WERROR =
endif

noinst_HEADERS = \
	include/utarray.h \
	include/uthash.h \
	include/utlist.h \
	include/json.hpp \
	tests_template/interval_profiling_block.yaml.erb \
	tests_template/interval_profiling_normal.yaml.erb \
	tests_template/interval_profiling_cuda_driver_api.yaml.erb \
	tests_template/interval_profiling_cuda_multictx.yaml.erb \
	tests_template/interval_profiling_cuda_multistream.yaml.erb \
	tests_template/interval_profiling_cuda_memcpy.yaml.erb \
	tests_template/interval_profiling_multithread.yaml.erb \
	tests_template/interval_profiling_kernel_name.yaml.erb \
	tests_template/interval_common.dust.erb \
	xprof_utils.cpp \
	xprof_utils.hpp

nodist_noinst_HEADERS = lttng/tracepoint_gen.h

lttng/tracepoint_gen.h: $(srcdir)/tracepoint_gen.rb
	mkdir -p lttng
	$(RUBY) $< 25 > $@

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./

%.h %.c: %.tp lttng/tracepoint_gen.h
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

BUILT_SOURCES = \
	thapi_metadata_tracepoints.h \
	thapi_metadata_tracepoints.c

thapi_metadata_SOURCES = \
	thapi_metadata.c

nodist_thapi_metadata_SOURCES = \
	thapi_metadata_tracepoints.h \
	thapi_metadata_tracepoints.c

thapi_metadata_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
thapi_metadata_LDFLAGS = $(LTTNG_UST_LIBS)

bin_PROGRAMS = thapi_metadata

bin_SCRIPTS = \
	babeltrace_thapi

.PHONY: version

version:
	git describe --abbrev=7 --dirty --always --tags > version || echo $(PACKAGE_VERSION) > version

data_DATA = \
	version \
	optparse_thapi.rb

CLEANFILES = \
	version \
	optparse_thapi.rb \
	lttng/tracepoint_gen.h \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	lttng_dummy_trace.sh \
	LTTng.rb \
	gen_babeltrace_lib_helper.rb \
	gen_babeltrace_model_helper.rb \
	tracepoint_gen.rb \
	yaml_ast.rb \
	yaml_ast_lttng.rb \
	gen_library_base.rb \
	gen_dummy_trace.rb \
	bt2.rb \
	bt_plugins/dust.rb \
	bt_plugins/comparator.rb \
	dump_trace_format.rb \
	gen_babeltrace_dispatchers.rb \
	gen_babeltrace_callbacks.rb \
	gen_babeltrace_emitter.rb \
	thapi_metadata_tracepoints.tp \
	command.rb \
	meta_parameters.rb \
	optparse_thapi.rb
