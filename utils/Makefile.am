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
	include/magic_enum.hpp \
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

include_HEADERS = thapi.h

lib_LTLIBRARIES = libThapi.la
nodist_libThapi_la_SOURCES = \
	thapi_tracepoints.h \
	thapi_tracepoints.c
libThapi_la_SOURCES = thapi_toggle.c
libThapi_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libThapi_la_LDFLAGS = $(LTTNG_UST_LIBS)

BTX_THAPI_GENERATED = \
	btx_thapi/metababel/metababel.h \
	btx_thapi/metababel/btx_component.h \
	btx_thapi/metababel/btx_component.c \
	btx_thapi/metababel/btx_upstream.h \
	btx_thapi/metababel/btx_upstream.c \
	btx_thapi/metababel/btx_downstream.h \
	btx_thapi/metababel/btx_downstream.c \
	btx_thapi/btx_main.c

$(BTX_THAPI_GENERATED): btx_thapi.yaml thapi_callbacks.c
	$(METABABEL) --enable-callbacks on_downstream --component-type FILTER \
		--upstream $(top_srcdir)/utils/btx_thapi.yaml --downstream $(top_srcdir)/utils/btx_thapi.yaml \
		-o btx_thapi

noinst_LTLIBRARIES = libThapiPlugin.la
nodist_libThapiPlugin_la_SOURCES = $(BTX_THAPI_GENERATED)
libThapiPlugin_la_SOURCES = thapi_callbacks.c
libThapiPlugin_la_CFLAGS = -fPIC -shared -Wall -Wextra -Wno-unused-parameter $(BABELTRACE2_CFLAGS) -I./btx_thapi -I$(top_srcdir)/utils/include

BUILT_SOURCES += \
	thapi_tracepoints.h \
	thapi_tracepoints.c \
	$(BTX_THAPI_GENERATED)

bin_SCRIPTS = \
	babeltrace_thapi

noinst_SCRIPTS = \
	test_wrapper_thapi_text_pretty.sh

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
	$(BTX_THAPI_GENERATED) \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	LTTng.rb \
	gen_babeltrace_lib_helper.rb \
	gen_babeltrace_model_helper.rb \
	tracepoint_gen.rb \
	yaml_ast.rb \
	yaml_ast_lttng.rb \
	gen_library_base.rb \
	dump_trace_format.rb \
	thapi_metadata_tracepoints.tp \
	thapi_tracepoints.tp \
	btx_thapi.yaml \
	command.rb \
	meta_parameters.rb \
	optparse_thapi.rb \
	thapi_log_to_bt_source_component.rb \
	thapi_text_pretty_set_ts.rb \
	test_wrapper_thapi_text_pretty.sh
