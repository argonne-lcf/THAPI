.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror -Wno-error=nonnull
else
  WERROR =
endif

SYCL_HDR = \
        $(srcdir)/include/sycl-tools.h

SYCL_NAMESPACES = sycl

SYCL_EXTRACTED = $(SYCL_NAMESPACES:=_api.yaml)

EXTRA_DIST = $(srcdir)/include 

# Generate the YAML representation of the API Header Files
$(SYCL_EXTRACTED): %_api.yaml: $(SYCL_HDR)
	h2yaml --compat-cast-to-yaml -c -Wc,-xc -Wc,-Imodified_include/ $(SYCL_HDR) > $(SYCL_EXTRACTED)

CLEANFILES = $(SYCL_EXTRACTED)

# SYCL Model is all the utils to generate the tracer and the probes
EXTRA_DIST += sycl_model.rb 

SYCL_MODEL = \
        $(srcdir)/sycl_model.rb \
        $(top_srcdir)/utils/gen_probe_base.rb \
        $(top_srcdir)/utils/yaml_ast.rb \
        $(top_srcdir)/utils/yaml_ast_lttng.rb \
        $(top_srcdir)/utils/LTTng.rb \
        $(SYCL_EXTRACTED)

# Generate the Probe (tracepoint)

SYCL_PROBES = sycl_tracepoints
SYCL_PROBES_TP = $(SYCL_PROBES:=.tp)
SYCL_PROBES_INCL = $(SYCL_PROBES:=.h)
SYCL_PROBES_SRC = $(SYCL_PROBES:=.c)

$(SYCL_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(SYCL_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
        $(SYCL_PROBES_TP) \
        $(SYCL_PROBES_INCL) \
        $(SYCL_PROBES_SRC) 

noinst_LTLIBRARIES = libsycltracepoints.la

nodist_libsycltracepoints_la_SOURCES = \
        $(SYCL_PROBES_INCL) \
        $(SYCL_PROBES_SRC)

libsycltracepoints_la_CPPFLAGS = -I$(srcdir)/include
libsycltracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libsycltracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

EXTRA_DIST += \
     	gen_sycl_tracepoints.rb

BUILT_SOURCES = \
       	$(SYCL_PROBES_INCL)

# Generate the tracer and associated libs

tracer_sycl.c: $(srcdir)/gen_sycl.rb $(SYCL_MODEL) $(SYCL_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
        tracer_sycl.c

EXTRA_DIST += \
        gen_sycl.rb


lib_LTLIBRARIES = libsycl.la

nodist_libsycl_la_SOURCES = \
	$(SYCL_PROBES_INCL) \
	tracer_sycl.c

libsycl_la_CPPFLAGS = -I$(srcdir)/include
libsycl_la_CFLAGS = -fPIC -Wall -Wextra $(WERROR) $(LTTNG_UST_CFLAGS)
libsycl_la_LDFLAGS = $(LTTNG_UST_LIBS)
libsycl_la_LIBADD  = libsycltracepoints.la $(LTTNG_UST_LIBS) -ldl

bin_SCRIPTS = \
	tracer_sycl.sh

# Babeltrace 2 Plugins

SYCL_LIB_GEN = \
        $(top_srcdir)/utils/gen_babeltrace_model_helper.rb \
        $(top_srcdir)/utils/gen_library_base.rb \
        $(srcdir)/gen_sycl_library_base.rb \
        $(top_srcdir)/utils/gen_probe_base.rb

EXTRA_DIST += \
        $(SYCL_LIB_GEN)

btx_sycl_model.yaml: $(srcdir)/gen_babeltrace_sycl_model.rb $(SYCL_LIB_GEN) $(SYCL_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
        gen_babeltrace_sycl_model.rb

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libSYCLInterval.la

BTX_SYCL_GENERATED = \
        btx_filter_sycl/metababel/metababel.h \
        btx_filter_sycl/metababel/btx_component.h \
        btx_filter_sycl/metababel/btx_component.c \
        btx_filter_sycl/metababel/btx_upstream.h \
        btx_filter_sycl/metababel/btx_upstream.c \
        btx_filter_sycl/metababel/btx_downstream.h \
        btx_filter_sycl/metababel/btx_downstream.c \
        btx_filter_sycl/btx_main.c

$(BTX_SYCL_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_sycl_model.yaml
	$(METABABEL) -u btx_sycl_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_sycl -p syclinterval -c interval --matching $(srcdir)/btx_syclmatching_model.yaml -i sycl-tools.h

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
        $(BTX_SYCL_GENERATED) \
        xprof_utils.hpp \
        btx_sycl_model.yaml

BUILT_SOURCES += \
        $(BTX_SYCL_GENERATED) \
        xprof_utils.hpp

nodist_libSYCLInterval_la_SOURCES = \
        $(BTX_SYCL_GENERATED) \
        xprof_utils.hpp

libSYCLInterval_la_SOURCES = \
        btx_syclinterval_callbacks.cpp \
        $(top_srcdir)/utils/include/magic_enum.hpp

libSYCLInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_filter_sycl
libSYCLInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libSYCLInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libSYCLInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module
