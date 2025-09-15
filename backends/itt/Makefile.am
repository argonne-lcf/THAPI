.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

ITT_HDR = \
        $(srcdir)/include/ittnotify.h

ITT_NAMESPACES = itt

ITT_EXTRACTED = $(ITT_NAMESPACES:=_api.yaml)

# Generate the YAML representation of the API Header Files
$(ITT_EXTRACTED): %_api.yaml: $(ITT_HDR)
	h2yaml -xc $(ITT_HDR) > $(ITT_EXTRACTED)

CLEANFILES = $(ITT_EXTRACTED)

# ITT Model is all the utils to generatr the tracer and the probes
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

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(ITT_PROBES_TP) \
	$(ITT_PROBES_INCL) \
	$(ITT_PROBES_SRC)

noinst_LTLIBRARIES = libitttracepoints.la

nodist_libitttracepoints_la_SOURCES = \
	$(ITT_PROBES_INCL) \
	$(ITT_PROBES_SRC)

libitttracepoints_la_CPPFLAGS = -I$(srcdir)/include
libitttracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libitttracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

# Generate the tracer and assciated libs

tracer_itt.c: $(srcdir)/gen_itt.rb $(ITT_MODEL) $(ITT_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	tracer_iit.c

lib_LTLIBRARIES = libTracerITT.la

nodist_libTracerITT_la_SOURCES = \
	$(ITT_PROBES_INCL) \
	tracer_itt.c

libTracerITT_la_CPPFLAGS = -I$(srcdir)/include
libTracerITT_la_CFLAGS = -Wall -Wextra $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerITT_la_LDFLAGS = $(LTTNG_UST_LIBS) -avoid-version -module
libTracerITT_la_LIBADD = libitttracepoints.la

bin_SCRIPTS = \
	tracer_itt.sh
