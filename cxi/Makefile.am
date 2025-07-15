if STRICT
  WERROR = -Werror
else
  WERROR =
endif

TRACEPOINT_GEN = \
	$(srcdir)/cxi_events.yaml

CXI_STATIC_PROBES = \
	cxi_sampling

CXI_STATIC_PROBES_TP = $(CXI_STATIC_PROBES:=.tp)

CXI_STATIC_PROBES_INCL = $(CXI_STATIC_PROBES:=.h)

CXI_STATIC_PROBES_SRC = $(CXI_STATIC_PROBES:=.c)

$(CXI_STATIC_PROBES_TP): %.tp: $(srcdir)/gen_cxi_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $< lttng_ust_$* > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

noinst_LTLIBRARIES = libcxitracepoints.la

nodist_libcxitracepoints_la_SOURCES = \
	$(CXI_STATIC_PROBES_INCL) \
	$(CXI_STATIC_PROBES_SRC)

libcxitracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libcxitracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libcxitracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

EXTRA_DIST = \
	cxi_events.yaml \
	gen_cxi_custom_probes.rb

CLEANFILES = \
	$(CXI_STATIC_PROBES_INCL) \
	$(CXI_STATIC_PROBES_TP) \
	$(CXI_STATIC_PROBES_SRC)

BUILT_SOURCES = \
	$(CXI_STATIC_PROBES_INCL)

libCXISampling_la_SOURCES = cxi_sampling_plugin.c

nodist_libCXISampling_la_SOURCES = \
	$(CXI_STATIC_PROBES_INCL)

libCXISampling_la_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling
libCXISampling_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(LTTNG_UST_CFLAGS)
libCXISampling_la_LDFLAGS = -avoid-version -module
libCXISampling_la_LIBADD = ../sampling/libThapiSampling.la libcxitracepoints.la -ldl $(LTTNG_UST_LIBS)

cxidir = $(pkglibdir)/cxi
cxi_LTLIBRARIES = libCXISampling.la

CLEANFILES += $(bin_PROGRAMS)


BTX_CXI_GENERATED = \
       	btx_filter_cxi/metababel/metababel.h \
        btx_filter_cxi/metababel/btx_component.h \
        btx_filter_cxi/metababel/btx_component.c \
        btx_filter_cxi/metababel/btx_upstream.h \
        btx_filter_cxi/metababel/btx_upstream.c \
        btx_filter_cxi/metababel/btx_downstream.h \
        btx_filter_cxi/metababel/btx_downstream.c \
        btx_filter_cxi/btx_main.c

EXTRA_DIST += \
        $(top_srcdir)/xprof/btx_interval_model.yaml \
        $(srcdir)/btx_cxi_model.yaml

$(BTX_CXI_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml $(srcdir)/btx_cxi_model.yaml
	$(METABABEL) -u $(srcdir)/btx_cxi_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_cxi -p cxiinterval -c interval

CLEANFILES += \
        $(BTX_CXI_GENERATED)

BUILT_SOURCES += \
        $(BTX_CXI_GENERATED)

nodist_libCXIInterval_la_SOURCES = \
        $(BTX_CXI_GENERATED)

libCXIInterval_la_SOURCES = \
        btx_cxiinterval_callbacks.cpp \
        btx_cxiinterval_callbacks.hpp

libCXIInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_cxi
libCXIInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCXIInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCXIInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libCXIInterval.la

