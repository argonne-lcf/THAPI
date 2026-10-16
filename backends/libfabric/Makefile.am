if STRICT
  WERROR = -Werror
else
  WERROR =
endif

#
# ----------------------------------------------------------------------
# Tracepoint provider
# ----------------------------------------------------------------------
#
# libfabric_events.yaml -> (gen_custom_probes.rb) -> .tp -> (lttng-gen-tp)
#   -> libfabric_tracepoints.{c,h}
#
# No h2yaml and no <name>_model.rb anywhere in this backend: the tracer is
# hand-written, so there is no header to parse.  backends/pytorch and
# backends/cxi are the two other backends built this way.
#

FI_STATIC_PROBES = libfabric_tracepoints

FI_STATIC_PROBES_TP = $(FI_STATIC_PROBES:=.tp)

FI_STATIC_PROBES_INCL = $(FI_STATIC_PROBES:=.h)

FI_STATIC_PROBES_SRC = $(FI_STATIC_PROBES:=.c)

$(FI_STATIC_PROBES_TP): %.tp: $(top_srcdir)/utils/gen_custom_probes.rb $(srcdir)/libfabric_events.yaml $(top_srcdir)/utils/gen_probe_base.rb
	$(RUBY) $< $(srcdir)/libfabric_events.yaml lttng_ust_fi > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

noinst_LTLIBRARIES = libfitracepoints.la

nodist_libfitracepoints_la_SOURCES = \
	$(FI_STATIC_PROBES_INCL) \
	$(FI_STATIC_PROBES_SRC)

libfitracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./
libfitracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libfitracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

EXTRA_DIST = \
	libfabric_events.yaml \
	$(top_srcdir)/utils/gen_custom_probes.rb \
	include

CLEANFILES = \
	$(FI_STATIC_PROBES_TP) \
	$(FI_STATIC_PROBES_INCL) \
	$(FI_STATIC_PROBES_SRC)

BUILT_SOURCES = \
	$(FI_STATIC_PROBES_INCL)

#
# ----------------------------------------------------------------------
# Tracer
# ----------------------------------------------------------------------
#
# Note what is absent compared to backends/pytorch: there is no dummy_libs
# stanza, because we never link against libfabric.  Every provider entry point
# is reached either through dlvsym(RTLD_NEXT, "fi_fabric", ...) or through a
# function pointer in a vtable the provider handed us, so the only thing
# libfabric contributes at build time is its headers.
#
# The headers are vendored under include/rdma/ so the backend builds on a
# machine with no libfabric installed -- same reason backends/mpi vendors
# mpi.h.  Update them with:
#     cp -r <libfabric>/include/rdma backends/libfabric/include/
#

bin_SCRIPTS = \
	tracer_libfabric.sh

lib_LTLIBRARIES = libTracerLibfabric.la

nodist_libTracerLibfabric_la_SOURCES = \
	$(FI_STATIC_PROBES_INCL)

libTracerLibfabric_la_SOURCES = \
	src/fit_hook.c \
	src/fit_addr.c \
	src/fit_lttng.c

libTracerLibfabric_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I$(srcdir)/src -I./
libTracerLibfabric_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerLibfabric_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -avoid-version -module
libTracerLibfabric_la_LIBADD = libfitracepoints.la

EXTRA_DIST += \
	src/fi_trace.h

#
# ----------------------------------------------------------------------
# Analysis: raw lttng_ust_fi trace -> interval stream
# ----------------------------------------------------------------------
#
# Three names here are load-bearing and fail confusingly rather than loudly if
# you change them:
#
#   -p fiinterval -c interval   utils/babeltrace_thapi.in:161 looks the
#                               component up as "filter.#{b}interval.interval"
#                               where b is the short backend name, "fi".
#   libFIInterval.la            utils/test_wrapper_thapi_text_pretty.sh.in:69
#                               builds lib"${be^^}"Interval.so.
#   $(pkglibdir)/bt2            babeltrace_thapi.in:151 scans exactly this one
#                               directory for plugins.
#

BTX_FI_GENERATED = \
	btx_filter_fi/metababel/metababel.h \
	btx_filter_fi/metababel/btx_component.h \
	btx_filter_fi/metababel/btx_component.c \
	btx_filter_fi/metababel/btx_upstream.h \
	btx_filter_fi/metababel/btx_upstream.c \
	btx_filter_fi/metababel/btx_downstream.h \
	btx_filter_fi/metababel/btx_downstream.c \
	btx_filter_fi/btx_main.c

EXTRA_DIST += \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	$(srcdir)/btx_libfabric_model.yaml

$(BTX_FI_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml $(srcdir)/btx_libfabric_model.yaml
	$(METABABEL) -u $(srcdir)/btx_libfabric_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_fi -p fiinterval -c interval

CLEANFILES += \
	$(BTX_FI_GENERATED)

BUILT_SOURCES += \
	$(BTX_FI_GENERATED)

nodist_libFIInterval_la_SOURCES = \
	$(BTX_FI_GENERATED)

libFIInterval_la_SOURCES = \
	btx_fiinterval_callbacks.cpp

libFIInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./ -I./btx_filter_fi
libFIInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libFIInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libFIInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libFIInterval.la

#
# ----------------------------------------------------------------------
# Enable pretty.print in babeltrace_thapi
# ----------------------------------------------------------------------
#
# The library name uses the *directory* name (libfabric), not the short
# backend name (fi) -- babeltrace_thapi.in:167 does the same for OpenCL,
# requiring babeltrace_opencl_lib for backend key "cl".
#

babeltrace_libfabric_lib.rb: $(srcdir)/gen_babeltrace_libfabric_lib.rb $(top_srcdir)/utils/gen_babeltrace_lib_helper.rb $(top_srcdir)/utils/gen_probe_base.rb $(srcdir)/btx_libfabric_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_babeltrace_libfabric_lib.rb

CLEANFILES += \
	babeltrace_libfabric_lib.rb

data_DATA = \
	babeltrace_libfabric_lib.rb