.DELETE_ON_ERROR:

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./

ZE_PROBES_TP = \
	ze_tracepoints.tp \
	zet_tracepoints.tp \
	zes_tracepoints.tp

ZE_PROBES_INCL = \
	ze_tracepoints.h \
	zet_tracepoints.h \
	zes_tracepoints.h

ZE_PROBES_SRC = \
	ze_tracepoints.c \
	zet_tracepoints.c \
	zes_tracepoints.c

ZE_STATIC_PROBES_TP = \
	ze_profiling.tp \
	ze_properties.tp

ZE_STATIC_PROBES_INCL = \
	ze_profiling.h \
	ze_properties.h

ZE_STATIC_PROBES_SRC = \
	ze_profiling.c \
	ze_properties.c

CLEANFILES = \
	$(ZE_PROBES_TP) \
	$(ZE_PROBES_INCL) \
	$(ZE_PROBES_SRC) \
	$(ZE_STATIC_PROBES_TP) \
	$(ZE_STATIC_PROBES_INCL) \
	$(ZE_STATIC_PROBES_SRC)

ML_FILES = \
	$(srcdir)/ze_meta_parameters.yaml \
	$(srcdir)/zes_meta_parameters.yaml \
	$(srcdir)/zet_meta_parameters.yaml

EXTRA_DIST = $(ML_FILES)
EXTRA_DIST += ze_events.yaml

ZE_EXTRACT = \
	$(srcdir)/extract_base.rb

EXTRA_DIST += extract_base.rb 

ZE_EXTRACTED = \
	ze_api.yaml \
	zet_api.yaml \
	zes_api.yaml

CLEANFILES += $(ZE_EXTRACTED)

ZE_MODEL = \
	$(srcdir)/ze_model.rb \
	$(ML_FILES) \
	$(srcdir)/ze_events.yaml \
	$(ZE_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb

EXTRA_DIST += \
	ze_model.rb \
	ze_events.yaml

ZE_EXTRA_HDR = \
	$(srcdir)/include/ze_ddi.h \
	$(srcdir)/include/zes_api.h \
	$(srcdir)/include/zes_ddi.h \
	$(srcdir)/include/zet_api.h \
	$(srcdir)/include/zet_ddi.h
ZE_HDR = \
	$(srcdir)/include/ze_api.h \
	$(ZE_EXTRA_HDR)

MODIFIED_EXTRA_HDR = \
	modified_include/ze_ddi.h \
	modified_include/zes_api.h \
	modified_include/zes_ddi.h \
	modified_include/zet_api.h \
	modified_include/zet_ddi.h
MODIFIED_ZE_HDR = \
	modified_include/ze_api.h \
	$(MODIFIED_EXTRA_HDR)

modified_include/ze_api.h: $(ZE_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

$(MODIFIED_EXTRA_HDR): modified_include/ze_api.h
	@if test -f $@; then \
	  touch $@; \
	else \
	  $(RM) modified_include/ze_api.h; \
	  $(MAKE) $(AM_MAKEFLAGS) modified_include/ze_api.h; \
	fi

clean-local:
	$(RM) -r modified_include

EXTRA_DIST += $(srcdir)/include headers.patch

ze_api.yaml: $(MODIFIED_ZE_HDR) $(ZE_EXTRACT) $(srcdir)/extract_ze.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_ze.rb

zet_api.yaml: $(MODIFIED_ZE_HDR) $(ZE_EXTRACT) $(srcdir)/extract_zet.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_zet.rb

zes_api.yaml: $(MODIFIED_ZE_HDR) $(ZE_EXTRACT) $(srcdir)/extract_zes.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_zes.rb

EXTRA_DIST += \
	extract_ze.rb \
	extract_zet.rb \
	extract_zes.rb

ze_tracepoints.tp: $(srcdir)/gen_ze_tracepoints.rb $(ZE_MODEL) $(srcdir)/gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_ze_tracepoints.rb > $@

zet_tracepoints.tp: $(srcdir)/gen_zet_tracepoints.rb $(ZE_MODEL) $(srcdir)/gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_zet_tracepoints.rb > $@

zes_tracepoints.tp: $(srcdir)/gen_zes_tracepoints.rb $(ZE_MODEL) $(srcdir)/gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_zes_tracepoints.rb > $@

ze_profiling.tp: $(srcdir)/gen_ze_custom_probes.rb $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_ze_custom_probes.rb lttng_ust_ze_profiling > $@

ze_properties.tp: $(srcdir)/gen_ze_custom_probes.rb $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_ze_custom_probes.rb lttng_ust_ze_properties > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

EXTRA_DIST += \
	gen_ze_custom_probes.rb \
	gen_ze_tracepoints.rb \
	gen_zet_tracepoints.rb \
	gen_zes_tracepoints.rb \
	gen_probe_base.rb

BUILT_SOURCES = $(ZE_PROBES_INCL) $(ZE_STATIC_PROBES_INCL) babeltrace_ze_callbacks.h

tracer_ze.c: $(srcdir)/gen_ze.rb $(srcdir)/tracer_ze_helpers.include.c $(ZE_MODEL) $(ZE_PROBES_INCL) $(ZE_STATIC_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_ze.rb > $@

CLEANFILES += tracer_ze.c

bin_SCRIPTS = tracer_ze.sh babeltrace_ze zeprof.sh

noinst_LTLIBRARIES = libzetracepoints.la

nodist_libzetracepoints_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	$(ZE_PROBES_SRC) \
	$(ZE_STATIC_PROBES_SRC)

libzetracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libzetracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerZE.la libZEProf.la

nodist_libTracerZE_la_SOURCES = \
	$(ZE_PROBES_INCL) \
	$(ZE_STATIC_PROBES_INCL) \
	tracer_ze.c

libTracerZE_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./
libTracerZE_la_CFLAGS = -Wall -Wextra -Werror
libTracerZE_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerZE_la_LIBADD = libzetracepoints.la

install-exec-hook::
	mkdir -p $(pkglibdir)
	$(RM) $(pkglibdir)/libze_loader.so.1 $(pkglibdir)/libze_loader.so
	ln -s $(libdir)/libTracerZE.so.1.0.0 $(pkglibdir)/libze_loader.so.1
	ln -s $(pkglibdir)/libze_loader.so.1 $(pkglibdir)/libze_loader.so

ZEPROF = \
	$(srcdir)/zeprof_callbacks.cpp.erb \
	$(srcdir)/zeprof_callbacks.h.erb \
	$(srcdir)/zeprof.c.erb \
	$(srcdir)/gen_zeprof.rb

zeprof.c: $(ZEPROF) babeltrace_ze_callbacks.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_zeprof.rb

zeprof_callbacks.cpp zeprof_callbacks.h: zeprof.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f zeprof.c; \
	  $(MAKE) $(AM_MAKEFLAGS) zeprof.c; \
	fi

CLEANFILES += \
	zeprof.c \
	zeprof_callbacks.cpp \
	zeprof_callbacks.h

nodist_libZEProf_la_SOURCES = \
	zeprof.c \
	zeprof_callbacks.cpp \
	zeprof_callbacks.h \
	babeltrace_ze_dispatchers.c
libZEProf_la_SOURCES = \
	babeltrace_ze.h

libZEProf_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libZEProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libZEProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libZEProf_la_LDFLAGS = -lstdc++ $(BABELTRACE2_LIBS) -avoid-version -module

EXTRA_DIST += $(ZEPROF)

tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceZE.la
install-data-hook::
	$(RM) -r $(DESTDIR)$(tmplibdir)

ZE_LIB_GEN = \
	$(srcdir)/gen_ze_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += $(ZE_LIB_GEN)

babeltrace_ze_callbacks.h: $(srcdir)/gen_babeltrace_ze_callbacks.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_ze_dispatchers.c: $(srcdir)/gen_babeltrace_ze_dispatchers.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_babeltrace_ze_callbacks.rb \
	gen_babeltrace_ze_dispatchers.rb

nodist_libBabeltraceZE_la_SOURCES = \
	babeltrace_ze_callbacks.h \
	babeltrace_ze_dispatchers.c
libBabeltraceZE_la_SOURCES = \
	babeltrace_ze.c \
	babeltrace_ze.h
libBabeltraceZE_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libBabeltraceZE_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libBabeltraceZE_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

ze_library.rb: $(srcdir)/gen_ze_library.rb $(ZE_LIB_GEN) $(ZE_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_ze_lib.rb: $(srcdir)/gen_babeltrace_ze_lib.rb $(ZE_LIB_GEN) $(ZE_MODEL)
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

data_DATA = $(ZE_BINDINGS)

EXTRA_DIST += \
	gen_ze_library.rb \
	gen_babeltrace_ze_lib.rb \
	gen_ze_refinements.rb \
	gen_ze_bindings.rb

CLEANFILES += $(ZE_BINDINGS)

