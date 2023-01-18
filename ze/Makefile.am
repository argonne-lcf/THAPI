.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

ZE_NAMESPACES = ze zet zes zel
ZE_STRUCTS_NAMESPACES = $(ZE_NAMESPACES:=_structs)

ZE_EXTRA_HDR = \
	$(srcdir)/include/ze_ddi.h \
	$(srcdir)/include/zes_api.h \
	$(srcdir)/include/zes_ddi.h \
	$(srcdir)/include/zet_api.h \
	$(srcdir)/include/zet_ddi.h \
	$(srcdir)/include/loader/ze_loader_api.h \
	$(srcdir)/include/layers/zel_tracing_api.h \
	$(srcdir)/include/layers/zel_tracing_ddi.h \
	$(srcdir)/include/layers/zel_tracing_register_cb.h \
	$(srcdir)/include/loader/ze_loader.h

ZE_HDR = \
	$(srcdir)/include/ze_api.h \
	$(ZE_EXTRA_HDR)

MODIFIED_ZE_EXTRA_HDR = \
	modified_include/ze_ddi.h \
	modified_include/zes_api.h \
	modified_include/zes_ddi.h \
	modified_include/zet_api.h \
	modified_include/zet_ddi.h \
	modified_include/loader/ze_loader_api.h \
	modified_include/layers/zel_tracing_api.h \
	modified_include/layers/zel_tracing_ddi.h \
	modified_include/layers/zel_tracing_register_cb.h \
	modified_include/loader/ze_loader.h

MODIFIED_ZE_HDR = \
	modified_include/ze_api.h \
	$(MODIFIED_ZE_EXTRA_HDR)

modified_include/ze_api.h: $(ZE_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

$(MODIFIED_ZE_EXTRA_HDR): modified_include/ze_api.h
	@if test -f $@; then \
	  touch $@; \
	else \
	  $(RM) modified_include/ze_api.h; \
	  $(MAKE) $(AM_MAKEFLAGS) modified_include/ze_api.h; \
	fi

clean-local:
	$(RM) -r modified_include
	$(RM) -r tests

EXTRA_DIST = $(srcdir)/include headers.patch

ZE_EXTRACTED = $(ZE_NAMESPACES:=_api.yaml)

$(ZE_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(MODIFIED_ZE_HDR) $(srcdir)/extract_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $<

CLEANFILES = $(ZE_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	$(ZE_NAMESPACES:%=extract_%.rb)

ML_FILES = \
	$(ZE_NAMESPACES:=_meta_parameters.yaml) \
	$(srcdir)/ze_events.yaml

EXTRA_DIST += $(ML_FILES)

ZE_MODEL = \
	$(srcdir)/ze_model.rb \
	$(ML_FILES) \
	$(ZE_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb

ze_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_ze_model.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += ze_babeltrace_model.yaml

EXTRA_DIST += \
	ze_model.rb \
	gen_babeltrace_ze_model.rb

ZE_PROBES = $(ZE_NAMESPACES:=_tracepoints) $(ZE_STRUCTS_NAMESPACES:=_tracepoints)
ZE_PROBES_TP = $(ZE_PROBES:=.tp)
ZE_PROBES_INCL = $(ZE_PROBES:=.h)
ZE_PROBES_SRC = $(ZE_PROBES:=.c)

ZE_STATIC_PROBES = \
	ze_profiling \
	ze_properties \
	ze_build

ZE_STATIC_PROBES_TP = $(ZE_STATIC_PROBES:=.tp)
ZE_STATIC_PROBES_INCL = $(ZE_STATIC_PROBES:=.h)
ZE_STATIC_PROBES_SRC = $(ZE_STATIC_PROBES:=.c)
ZE_GEN_TRACEPOINTS = $(ZE_PROBES:%=gen_%.rb)

$(ZE_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(ZE_MODEL) $(srcdir)/gen_probe_base.rb $(srcdir)/ze.h.include
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

$(ZE_STATIC_PROBES_TP): %.tp: $(srcdir)/gen_ze_custom_probes.rb $(ZE_MODEL) $(srcdir)/ze.h.include
	SRC_DIR=$(srcdir) $(RUBY) $< lttng_ust_$* > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(ZE_PROBES_TP) \
	$(ZE_PROBES_INCL) \
	$(ZE_PROBES_SRC) \
	$(ZE_STATIC_PROBES_TP) \
	$(ZE_STATIC_PROBES_INCL) \
	$(ZE_STATIC_PROBES_SRC)

EXTRA_DIST += \
	gen_probe_base.rb \
	$(ZE_GEN_TRACEPOINTS) \
	gen_ze_custom_probes.rb

BUILT_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL)

tracer_ze.c: $(srcdir)/gen_ze.rb $(srcdir)/tracer_ze_helpers.include.c $(srcdir)/ze.h.include $(ZE_MODEL) $(ZE_PROBES_INCL) $(ZE_STATIC_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_ze.rb \
	tracer_ze_helpers.include.c

CLEANFILES += tracer_ze.c

zeprof.sh: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages=["ze"] $< > $@
	chmod a+x $@

CLEANFILES += zeprof.sh

bin_SCRIPTS = \
	tracer_ze.sh \
	babeltrace_ze \
	zeprof.sh

noinst_LTLIBRARIES = libzetracepoints.la

nodist_libzetracepoints_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	$(ZE_PROBES_SRC) \
	$(ZE_STATIC_PROBES_SRC)

libzetracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libzetracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerZE.la libZEInterval.la

nodist_libTracerZE_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	tracer_ze.c

libTracerZE_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libTracerZE_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerZE_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerZE_la_LIBADD = libzetracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ze
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerZE.so.0.0.0 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so
	-rmdir $(DESTDIR)$(pkglibdir)/ze

tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceZE.la
install-data-hook::
	$(RM) -r $(DESTDIR)$(tmplibdir)

ZE_LIB_GEN = \
	$(srcdir)/gen_ze_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += $(ZE_LIB_GEN)

babeltrace_ze_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb ze_babeltrace_model.yaml ze.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ ze > $@

BUILT_SOURCES += babeltrace_ze_callbacks.h

babeltrace_ze_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ ze > $@

EXTRA_DIST += ze.h.include

CLEANFILES += \
	babeltrace_ze_callbacks.h \
	babeltrace_ze_dispatchers.c

nodist_libBabeltraceZE_la_SOURCES = \
	babeltrace_ze_callbacks.h \
	babeltrace_ze_dispatchers.c

libBabeltraceZE_la_SOURCES = \
	babeltrace_ze.c \
	babeltrace_ze.h

libBabeltraceZE_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libBabeltraceZE_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libBabeltraceZE_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

ze_library.rb: $(srcdir)/gen_ze_library.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_ze_lib.rb: $(srcdir)/gen_babeltrace_ze_lib.rb $(ZE_LIB_GEN) $(ZE_MODEL) ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

ze_refinements.rb: $(srcdir)/gen_ze_refinements.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

ze_bindings.rb: $(srcdir)/gen_ze_bindings.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

ZE_BINDINGS = \
	ze_library.rb \
	babeltrace_ze_lib.rb \
	ze_refinements.rb \
	ze_bindings.rb

EXTRA_DIST += \
	ze_bindings_base.rb \
	gen_ze_library.rb \
	gen_babeltrace_ze_lib.rb \
	gen_ze_refinements.rb \
	gen_ze_bindings.rb

CLEANFILES += $(ZE_BINDINGS)

data_DATA = \
	$(ZE_BINDINGS) \
	ze_bindings_base.rb \
	babeltrace_zeprofiling_apis.txt

babeltrace_zeinterval_callbacks.cpp: $(srcdir)/gen_zeinterval_callbacks.rb $(srcdir)/zeinterval_callbacks.cpp.erb ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< callbacks > $@

babeltrace_zeprofiling_apis.txt: $(srcdir)/gen_zeinterval_callbacks.rb $(srcdir)/zeinterval_callbacks.cpp.erb ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< apis > $@

babeltrace_zeinterval_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb ze_babeltrace_model.yaml ze.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ zeinterval > $@

babeltrace_zeinterval_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ zeinterval > $@

babeltrace_zeinterval_emitter.h: $(top_srcdir)/utils/gen_babeltrace_emitter.rb $(top_srcdir)/xprof/interval_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ zeinterval > $@

babeltrace_zeinterval.h: $(top_srcdir)/xprof/interval.h.erb
	$(ERB) namespace="zeinterval" $< > $@

babeltrace_zeinterval.c: $(top_srcdir)/xprof/interval.c.erb 
	$(ERB) namespace="zeinterval" $< > $@

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

EXTRA_DIST += \
	gen_zeinterval_callbacks.rb \
	zeinterval_callbacks.cpp.erb

CLEANFILES += \
	babeltrace_zeinterval_callbacks.cpp \
	babeltrace_zeprofiling_apis.txt \
	babeltrace_zeinterval_callbacks.h \
	babeltrace_zeinterval_dispatchers.c \
	babeltrace_zeinterval_emitter.h \
	babeltrace_zeinterval.h \
	babeltrace_zeinterval.c \
	xprof_utils.hpp \
	xprof_utils.cpp

BUILT_SOURCES += \
	babeltrace_zeinterval_callbacks.h \
	babeltrace_zeinterval.h \
	babeltrace_zeinterval_emitter.h \
	xprof_utils.hpp

nodist_libZEInterval_la_SOURCES = \
	babeltrace_zeinterval.c \
	babeltrace_zeinterval.h \
	babeltrace_zeinterval_callbacks.h \
	babeltrace_zeinterval_dispatchers.c \
	babeltrace_zeinterval_callbacks.cpp \
	babeltrace_zeinterval_emitter.h \
	xprof_utils.hpp \
	xprof_utils.cpp 

libZEInterval_la_SOURCES = \
	zeinterval_callbacks.hpp

libZEInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libZEInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libZEInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libZEInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

tests/.tag:
	mkdir -p tests
	touch tests/.tag

CLEANFILES += tests/.tag

tests/ze_dummy_trace.yaml: $(top_srcdir)/utils/gen_dummy_trace.rb ze_babeltrace_model.yaml tests/.tag
	mkdir -p tests
	$(RUBY) $< ze_babeltrace_model.yaml > $@

# To run test we need to catageory of file:
# - One are the `yaml` file who containt the traces
# - The other is the `dust` file who containt the babeltrace processing graph
#   The dust file will use the `yaml` file.

# Test by themself are splited into two type:
# - The one who are common to each backend.
# - The specific for this backend

# We generate lot of file using:
#https://www.gnu.org/software/make/manual/html_node/Static-Usage.html

TRACE_COMMON = \
	tests/interval_profiling_normal.yaml

$(TRACE_COMMON): tests/%.yaml: $(top_srcdir)/utils/tests_template/%.yaml.erb tests/.tag
	$(ERB) be="ze" $< > $@

DUST_COMMON = $(TRACE_COMMON:.yaml=.dust)

$(DUST_COMMON): tests/interval_%.dust: $(top_srcdir)/utils/tests_template/interval_common.dust.erb tests/.tag
	$(ERB) be="ze" interval="zeinterval" namespace="$@" $< > $@

TRACE_SPECIFC = \
	tests/API_call.yaml \
	tests/profiling_fast.yaml \
	tests/profiling_interleave_process.yaml \
	tests/interval_API_call.yaml \
	tests/interval_profiling_normal.yaml \
	tests/interval_profiling_fast.yaml \
	tests/interval_profiling_interleave_process.yaml

DUST_SPECIFIC = \
	tests/interval_API_call.dust \
	tests/interval_profiling_fast.dust \
	tests/interval_profiling_interleave_process.dust

$(DUST_SPECIFIC): tests/interval_%.dust: tests/interval.dust.erb
	$(ERB) namespace="$@" $< > $@

TESTS = \
	tests/ze_dummy_trace.dust \
	$(DUST_COMMON) \
	$(DUST_SPECIFIC)

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh

check_DATA = \
	test_wrapper.sh \
	ze_library.rb \
	tests/ze_dummy_trace.yaml \
	$(TRACE_COMMON) \
	$(TRACE_SPECIFC)

EXTRA_DIST += \
	$(TRACE_SPECIFC) \
	tests/ze_dummy_trace.dust \
	tests/interval.dust.erb

CLEANFILES += \
	tests/ze_dummy_trace.yaml \
	$(DUST_COMMON) \
	$(DUST_SPECIFIC) \
	$(TRACE_COMMON) 
