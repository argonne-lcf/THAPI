.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

ZE_NAMESPACES = ze zet zes zel zex
ZE_STRUCTS_NAMESPACES = $(ZE_NAMESPACES:=_structs)

ZE_HDR_ROOT = \
	ze_api.h \
	ze_ddi.h \
	ze_ddi_ver.h \
	zes_api.h \
	zes_ddi.h \
	zes_ddi_ver.h \
	zet_api.h \
	zet_ddi.h \
	zet_ddi_ver.h \
	zex_api.h \
	loader/ze_loader_api.h \
	layers/zel_tracing_api.h \
	layers/zel_tracing_ddi.h \
	layers/zel_tracing_ddi_ver.h \
	layers/zel_tracing_register_cb.h \
	loader/ze_loader.h

ZE_HDR = $(ZE_HDR_ROOT:%=$(srcdir)/include/%)
MODIFIED_ZE_HDR = $(ZE_HDR_ROOT:%=modified_include/%)

BTX_ZE_GENERATED = \
	btx_filter_ze/metababel/metababel.h \
	btx_filter_ze/metababel/btx_component.h \
	btx_filter_ze/metababel/btx_component.c \
	btx_filter_ze/metababel/btx_upstream.h \
	btx_filter_ze/metababel/btx_upstream.c \
	btx_filter_ze/metababel/btx_downstream.h \
	btx_filter_ze/metababel/btx_downstream.c \
	btx_filter_ze/btx_main.c

EXTRA_DIST = \
	$(top_srcdir)/utils/babeltrace_to_metababel_model.rb \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	btx_zematching_model.yaml \
	babeltrace_zeprofiling_apis.txt

CLEANFILES = btx_ze_model.yaml

btx_ze_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb ze_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_ZE_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_zematching_model.yaml btx_ze_model.yaml
	$(METABABEL) -u btx_ze_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_ze -p zeinterval -c interval --matching $(srcdir)/btx_zematching_model.yaml -i ze.h.include

$(MODIFIED_ZE_HDR) &: $(ZE_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

clean-local:
	$(RM) -r modified_include

EXTRA_DIST += \
	$(srcdir)/include \
	headers.patch

ZE_EXTRACTED = $(ZE_NAMESPACES:=_api.yaml)

$(ZE_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(MODIFIED_ZE_HDR) $(srcdir)/extract_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $<

CLEANFILES += $(ZE_EXTRACTED)

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
	$(top_srcdir)/utils/yaml_ast.rb \
	$(top_srcdir)/utils/yaml_ast_lttng.rb \
	$(top_srcdir)/utils/meta_parameters.rb \
	$(top_srcdir)/utils/LTTng.rb

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
	ze_sampling \
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

bin_SCRIPTS = \
	tracer_ze.sh

noinst_LTLIBRARIES = libzetracepoints.la

nodist_libzetracepoints_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	$(ZE_PROBES_SRC) \
	$(ZE_STATIC_PROBES_SRC)

libzetracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libzetracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libzetracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerZE.la libZEInterval.la

nodist_libTracerZE_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	tracer_ze.c

libTracerZE_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I$(srcdir)/include -I$(top_srcdir)/utils -I./
libTracerZE_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerZE_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS) ../sampling/libThapiSampling.la
libTracerZE_la_LIBADD = libzetracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ze
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerZE.so.0.0.0 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libZEInterval.so $(DESTDIR)$(pkglibdir)/bt2/libZEInterval.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/ze/libze_loader.so.1 $(DESTDIR)$(pkglibdir)/ze/libze_loader.so
	-rmdir $(DESTDIR)$(pkglibdir)/ze
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libZEInterval.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

tmplibdir = $(libdir)/tmp

install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

ZE_LIB_GEN = \
	$(top_srcdir)/utils/gen_babeltrace_model_helper.rb \
	$(top_srcdir)/utils/gen_library_base.rb \
	$(srcdir)/gen_ze_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += $(ZE_LIB_GEN)

EXTRA_DIST += ze.h.include

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

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
	$(BTX_ZE_GENERATED) \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_ZE_GENERATED)

nodist_libZEInterval_la_SOURCES = \
	$(BTX_ZE_GENERATED) \
	xprof_utils.hpp

libZEInterval_la_SOURCES = \
	btx_zeinterval_callbacks.cpp \
	btx_zeinterval_callbacks.hpp

libZEInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_ze
libZEInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libZEInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libZEInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

TRACE_COMMON = \
	tests/interval_profiling_normal.thapi_text_pretty \
	tests/interval_profiling_multithread.thapi_text_pretty \
	tests/interval_profiling_API_call.thapi_text_pretty \
	tests/interval_profiling_fast.thapi_text_pretty \
	tests/interval_profiling_interleave_process.thapi_text_pretty \
	tests/interval_profiling_ignore.thapi_text_pretty

BTX_ZE_GENERATED_SOURCE_TEST = \
        btx_source_ze_test/metababel/metababel.h \
        btx_source_ze_test/metababel/btx_component.h \
        btx_source_ze_test/metababel/btx_component.c \
        btx_source_ze_test/metababel/btx_downstream.h \
        btx_source_ze_test/metababel/btx_downstream.c \
        btx_source_ze_test/btx_main.c

$(BTX_ZE_GENERATED_SOURCE_TEST) &: btx_ze_model.yaml
	$(METABABEL) -d btx_ze_model.yaml -t SOURCE  -o btx_source_ze_test -p zetest -c test  -i ze.h.include

noinst_LTLIBRARIES += libtestzesource.la
nodist_libtestzesource_la_SOURCES = $(BTX_ZE_GENERATED_SOURCE_TEST)
libtestzesource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_ze_test/
libtestzesource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

TRACE_OUT = $(TRACE_COMMON:.thapi_text_pretty=.bt_text_pretty)

TESTS = $(TRACE_COMMON)

TEST_EXTENSIONS = .thapi_text_pretty
THAPI_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_TEXT_PRETTY_LOG_FLAGS = ze ze ./btx_source_ze_test/ .libs/libtestzesource.a

check_DATA = \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

EXTRA_DIST += \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

CLEANFILES += \
	$(TRACE_COMMON) \
	$(BTX_ZE_GENERATED_SOURCE_TEST)
