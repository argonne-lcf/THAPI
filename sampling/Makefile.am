if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./


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

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL) \
	$(SAMPLING_STATIC_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

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

libThapiSampling_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -I$(top_srcdir)/utils/include
libThapiSampling_la_LDFLAGS = -lpthread -version-info 1:0:0
libThapiSampling_la_LIBADD = libtracepoints.la

lib_LTLIBRARIES = libThapiSampling.la
