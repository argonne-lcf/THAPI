noinst_HEADERS = \
	lttng/tracepoint.h \
	include/utarray.h \
	include/uthash.h \
	include/utlist.h \
	tests_template/interval_profiling_block.yaml.erb \
	tests_template/interval_profiling_normal.yaml.erb \
	tests_template/interval_common.dust.erb \
	xprof_utils.cpp \
	xprof_utils.hpp

nodist_noinst_HEADERS = lttng/tracepoint_gen.h

lttng/tracepoint_gen.h: $(srcdir)/tracepoint_gen.rb
	mkdir -p lttng
	$(RUBY) $< 25 > $@

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./

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

CLEANFILES = \
	lttng/tracepoint_gen.h \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	lttng_dummy_trace.sh \
	LTTng.rb \
	tracepoint_gen.rb \
	yaml_ast.rb \
	gen_dummy_trace.rb \
	bt2.rb \
	bt_plugins/dust.rb \
	bt_plugins/comparator.rb \
	dump_trace_format.rb \
	gen_babeltrace_dispatchers.rb \
	gen_babeltrace_callbacks.rb \
	thapi_metadata_tracepoints.tp
