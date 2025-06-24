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

LTTNG_FLAGS= -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) \
             -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./

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

bin_PROGRAMS = dlinfo_wrapper
dlinfo_wrapper_SOURCES = dlinfo_wrapper.c
dlinfo_wrapper_CFLAGS = -Wall -Wextra

bin_PROGRAMS += thapi_metadata

include_HEADERS = thapi.h

lib_LTLIBRARIES = libThapi.la
nodist_libThapi_la_SOURCES = \
	thapi_toggle_tracepoints.h \
	thapi_toggle_tracepoints.c
libThapi_la_SOURCES = thapi_toggle.c
libThapi_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libThapi_la_LDFLAGS = $(LTTNG_UST_LIBS)

BTX_THAPI_TOGGLE_GENERATED = \
	btx_thapi_toggle/metababel/metababel.h \
	btx_thapi_toggle/metababel/btx_component.h \
	btx_thapi_toggle/metababel/btx_component.c \
	btx_thapi_toggle/metababel/btx_upstream.h \
	btx_thapi_toggle/metababel/btx_upstream.c \
	btx_thapi_toggle/metababel/btx_downstream.h \
	btx_thapi_toggle/metababel/btx_downstream.c \
	btx_thapi_toggle/btx_main.c

$(BTX_THAPI_TOGGLE_GENERATED): $(srcdir)/btx_thapi_toggle.yaml $(srcdir)/thapi_toggle_callbacks.cpp
	$(METABABEL) --enable-callbacks on_downstream --component-type FILTER \
		--upstream $(srcdir)/btx_thapi_toggle.yaml --downstream $(srcdir)/btx_thapi_toggle.yaml \
		-o btx_thapi_toggle

noinst_LTLIBRARIES = libThapiPlugin.la
nodist_libThapiPlugin_la_SOURCES = $(BTX_THAPI_TOGGLE_GENERATED)
libThapiPlugin_la_SOURCES = thapi_toggle_callbacks.cpp
libThapiPlugin_la_CFLAGS = -fPIC -shared -Wall -Wextra -Wno-unused-parameter $(BABELTRACE2_CFLAGS) \
                           -I./btx_thapi_toggle -I$(top_srcdir)/utils/include
libThapiPlugin_la_CXXFLAGS = -fPIC -shared -Wall -Wextra -Wno-unused-parameter $(BABELTRACE2_CFLAGS) \
                             -I./btx_thapi_toggle -I$(top_srcdir)/utils/include

BUILT_SOURCES += \
	thapi_toggle_tracepoints.h \
	thapi_toggle_tracepoints.c \
	$(BTX_THAPI_TOGGLE_GENERATED)

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
	$(BTX_THAPI_TOGGLE_GENERATED) \
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
	thapi_toggle_tracepoints.tp \
	btx_thapi_toggle.yaml \
	command.rb \
	meta_parameters.rb \
	optparse_thapi.rb \
	thapi_log_to_bt_source_component.rb \
	thapi_text_pretty_set_ts.rb \
	test_wrapper_thapi_text_pretty.sh
