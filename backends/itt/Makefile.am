.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror -Wno-error=nonnull
else
  WERROR =
endif

ITT_HDR = \
	$(srcdir)/include/ittnotify.h

ITT_NAMESPACES = itt

ITT_EXTRACTED = $(ITT_NAMESPACES:=_api.yaml)

# Generate the YAML representation of the API Header Files
#$(ITT_EXTRACTED): %_api.yaml: $(ITT_HDR)
#	h2yaml -c -Wc,-xc $(ITT_HDR) > $(ITT_EXTRACTED)
#
#CLEANFILES = $(ITT_EXTRACTED)

ITT_NAMESPACES = itt

ITT_EXTRACTED = $(ITT_NAMESPACES:=_api.yaml)

$(ITT_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(srcdir)/extract_base.rb
	ENABLE_CLANG_PARSER=$(ENABLE_CLANG_PARSER) $(RUBY) $<

CLEANFILES = $(ITT_EXTRACTED)

EXTRA_DIST = \
	extract_base.rb \
	$(ITT_NAMESPACES:%=extract_%.rb)

# ITT Model is all the utils to generate the tracer and the probes
EXTRA_DIST += \
	$(srcdir)/include \
	itt_model.rb \
	itt_meta_parameters.yaml

ITT_MODEL = \
	$(srcdir)/itt_model.rb \
	$(srcdir)/itt_meta_parameters.yaml \
	$(top_srcdir)/utils/gen_probe_base.rb \
	$(top_srcdir)/utils/yaml_ast.rb \
	$(top_srcdir)/utils/yaml_ast_lttng.rb \
	$(top_srcdir)/utils/meta_parameters.rb \
	$(top_srcdir)/utils/LTTng.rb \
	$(ITT_EXTRACTED)

# Generate the Probe (tracepoint)

ITT_PROBES = itt_tracepoints
ITT_PROBES_TP = $(ITT_PROBES:=.tp)
ITT_PROBES_INCL = $(ITT_PROBES:=.h)
ITT_PROBES_SRC = $(ITT_PROBES:=.c)

$(ITT_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(ITT_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

ITT_STATIC_PROBES = itt_metadata
ITT_STATIC_PROBES_TP = $(ITT_STATIC_PROBES:=.tp)
ITT_STATIC_PROBES_INCL = $(ITT_STATIC_PROBES:=.h)
ITT_STATIC_PROBES_SRC = $(ITT_STATIC_PROBES:=.c)

$(ITT_STATIC_PROBES_TP): %.tp: $(top_srcdir)/utils/gen_custom_probes.rb $(srcdir)/itt_events.yaml $(ITT_MODEL)
	$(RUBY) $< $(srcdir)/itt_events.yaml lttng_ust_$* ittnotify.h > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(ITT_PROBES_TP) \
	$(ITT_PROBES_INCL) \
	$(ITT_PROBES_SRC) \
	$(ITT_STATIC_PROBES_TP) \
	$(ITT_STATIC_PROBES_INCL) \
	$(ITT_STATIC_PROBES_SRC)

noinst_LTLIBRARIES = libitttracepoints.la

nodist_libitttracepoints_la_SOURCES = \
	$(ITT_PROBES_INCL) \
	$(ITT_PROBES_SRC) \
	$(ITT_STATIC_PROBES_INCL) \
	$(ITT_STATIC_PROBES_SRC)

libitttracepoints_la_CPPFLAGS = -I$(srcdir)/include
libitttracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS) -DINTEL_NO_MACRO_BODY
libitttracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

EXTRA_DIST += \
	gen_itt_tracepoints.rb \
	itt_events.yaml

BUILT_SOURCES = \
	$(ITT_PROBES_INCL) \
	$(ITT_STATIC_PROPBES_INCL)

# Generate the tracer and associated libs

tracer_itt.c: $(srcdir)/gen_itt.rb $(ITT_MODEL) $(ITT_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	tracer_itt.c

EXTRA_DIST += \
	gen_itt.rb

lib_LTLIBRARIES = libittnotify.la

nodist_libittnotify_la_SOURCES = \
	$(ITT_PROBES_INCL) \
	$(ITT_STATIC_PROBES_INCL) \
	tracer_itt.c

libittnotify_la_CPPFLAGS = -I$(srcdir)/include
libittnotify_la_CFLAGS = -fPIC -Wall -Wextra $(WERROR) $(LTTNG_UST_CFLAGS)
libittnotify_la_LDFLAGS = $(LTTNG_UST_LIBS)
libittnotify_la_LIBADD  = libitttracepoints.la $(LTTNG_UST_LIBS) -ldl

bin_SCRIPTS = \
	tracer_itt.sh

# Babeltrace 2 Plugins

ITT_LIB_GEN = \
	$(top_srcdir)/utils/gen_babeltrace_model_helper.rb \
	$(top_srcdir)/utils/gen_library_base.rb \
	$(srcdir)/gen_itt_library_base.rb

EXTRA_DIST += \
	$(ITT_LIB_GEN)

btx_itt_model.yaml: $(srcdir)/gen_babeltrace_itt_model.rb $(ITT_LIB_GEN) $(ITT_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_babeltrace_itt_model.rb

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libITTInterval.la

BTX_ITT_GENERATED = \
	btx_filter_itt/metababel/metababel.h \
	btx_filter_itt/metababel/btx_component.h \
	btx_filter_itt/metababel/btx_component.c \
	btx_filter_itt/metababel/btx_upstream.h \
	btx_filter_itt/metababel/btx_upstream.c \
	btx_filter_itt/metababel/btx_downstream.h \
	btx_filter_itt/metababel/btx_downstream.c \
	btx_filter_itt/btx_main.c

$(BTX_ITT_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_itt_model.yaml
	$(METABABEL) -u btx_itt_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_itt -p ittinterval -c interval -i ittnotify.h

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
	$(BTX_ITT_GENERATED) \
	xprof_utils.hpp \
	btx_itt_model.yaml

BUILT_SOURCES += \
	$(BTX_ITT_GENERATED) \
	xprof_utils.hpp

nodist_libITTInterval_la_SOURCES = \
	$(BTX_ITT_GENERATED) \
	xprof_utils.hpp

libITTInterval_la_SOURCES = \
	btx_ittinterval_callbacks.cpp \
	$(top_srcdir)/utils/include/magic_enum.hpp

libITTInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_filter_itt
libITTInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libITTInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libITTInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module
