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

$(SAMPLING_STATIC_PROBES_TP): %.tp: $(top_srcdir)/utils/gen_custom_probes.rb $(srcdir)/sampling_events.yaml
	$(RUBY) $< $(srcdir)/sampling_events.yaml lttng_ust_$* > $@

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
	$(top_srcdir)/utils/gen_custom_probes.rb

CLEANFILES = \
	$(SAMPLING_STATIC_PROBES_INCL) \
	$(SAMPLING_STATIC_PROBES_TP) \
	$(SAMPLING_STATIC_PROBES_SRC)

BUILT_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL)

libThapiSampling_la_SOURCES = \
	thapi_sampling.h \
	thapi_sampling.c

libThapiSampling_la_CPPFLAGS = -I$(top_srcdir)/utils/include
libThapiSampling_la_CFLAGS = -Wall -Wextra $(WERROR)
libThapiSampling_la_LDFLAGS = -lpthread -version-info 1:0:0 $(LTTNG_UST_LIBS)

bin_PROGRAMS = thapi_sampling_daemon

thapi_sampling_daemon_SOURCES = \
	thapi_sampling.h \
	thapi_sampling_daemon.cpp

thapi_sampling_daemon_CPPFLAGS = -I$(top_srcdir)/utils/include
thapi_sampling_daemon_CXXFLAGS = -Wall -Wextra $(WERROR)

libHeartbeatSampling_la_SOURCES = heartbeat_sampling_plugin.c

nodist_libHeartbeatSampling_la_SOURCES = \
	$(SAMPLING_STATIC_PROBES_INCL)

libHeartbeatSampling_la_CPPFLAGS = -I$(top_srcdir)/utils/include
libHeartbeatSampling_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(LTTNG_UST_CFLAGS)
libHeartbeatSampling_la_LDFLAGS = -avoid-version -module
libHeartbeatSampling_la_LIBADD = libThapiSampling.la libsamplingtracepoints.la -ldl $(LTTNG_UST_LIBS)

samplingdir = $(pkglibdir)/sampling
sampling_LTLIBRARIES = libThapiSampling.la libHeartbeatSampling.la

CLEANFILES += $(bin_PROGRAMS)
