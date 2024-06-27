if STRICT
  WERROR = -Werror
else
  WERROR =
endif

TRACEPOINT_GEN = \
	$(srcdir)/sampling_events.yaml

SAMPLING_STATIC_PROBES = \
	sampling

SAMPLING_STATIC_PROBES_TP = $(SAMPLING_STATIC_PROBES:=.tp)

SAMPLING_STATIC_PROBES_INCL = $(SAMPLING_STATIC_PROBES:=.h)

SAMPLING_STATIC_PROBES_SRC = $(SAMPLING_STATIC_PROBES:=.c)

$(SAMPLING_STATIC_PROBES_TP): %.tp: $(srcdir)/gen_sampling_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $< lttng_ust_$* > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

noinst_LTLIBRARIES = libsamplingtracepoints.la

nodist_libsamplingtracepoints_la_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL) \
	$(SAMPLING_STATIC_PROBES_SRC)

libsamplingtracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libsamplingtracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libsamplingtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)


EXTRA_DIST = \
	sampling_events.yaml \
	gen_sampling_custom_probes.rb

CLEANFILES = \
	$(SAMPLING_STATIC_PROBES_INCL) \
	$(SAMPLING_STATIC_PROBES_TP) \
	$(SAMPLING_STATIC_PROBES_SRC)

BUILT_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL)

nodist_libThapiSampling_la_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL)

libThapiSampling_la_SOURCES = \
	thapi_sampling.h \
	thapi_sampling.c

libThapiSampling_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -I$(top_srcdir)/utils/include $(LTTNG_UST_CFLAGS)
libThapiSampling_la_LDFLAGS = -lpthread -version-info 1:0:0 $(LTTNG_UST_LIBS)
libThapiSampling_la_LIBADD = libsamplingtracepoints.la

lib_LTLIBRARIES = libThapiSampling.la
