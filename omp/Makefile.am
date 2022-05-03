.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./modified_include -I../utils -I./

OMP_HDR = \
	$(srcdir)/include/omp-tools.h

MODIFIED_OMP_HDR = \
	modified_include/omp-tools.h

modified_include/omp-tools.h: $(OMP_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

clean-local:
	$(RM) -r modified_include
	$(RM) -r tests

EXTRA_DIST = $(srcdir)/include headers.patch

OMP_NAMESPACES = ompt

OMP_EXTRACTED = $(OMP_NAMESPACES:=_api.yaml)

$(OMP_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(MODIFIED_OMP_HDR) $(srcdir)/extract_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $<

CLEANFILES = $(OMP_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	$(OMP_NAMESPACES:%=extract_%.rb)

OMP_MODEL = \
	$(srcdir)/ompt_model.rb \
	$(srcdir)/ompt_meta_parameters.yaml \
	$(srcdir)/gen_probe_base.rb \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb \
	$(OMP_EXTRACTED)

OMP_LIB_GEN = \
	$(srcdir)/gen_omp_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += \
	$(OMP_LIB_GEN) \
	$(srcdir)/ompt_meta_parameters.yaml

omp_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_omp_model.rb $(OMP_LIB_GEN) $(OMP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += omp_babeltrace_model.yaml

EXTRA_DIST += \
	ompt_model.rb \
	gen_babeltrace_omp_model.rb

OMP_PROBES = ompt_tracepoints
OMP_PROBES_TP = $(OMP_PROBES:=.tp)
OMP_PROBES_INCL = $(OMP_PROBES:=.h)
OMP_PROBES_SRC = $(OMP_PROBES:=.c)

$(OMP_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(OMP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(OMP_PROBES_TP) \
	$(OMP_PROBES_INCL) \
	$(OMP_PROBES_SRC)

EXTRA_DIST += \
	gen_ompt_tracepoints.rb

BUILT_SOURCES = \
	$(MODIFIED_OMP_HDR) \
	$(OMP_PROBES_INCL)

tracer_ompt.c: $(srcdir)/gen_ompt.rb tracer_ompt_helpers.include.c $(OMP_MODEL) $(OMP_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

tracer_ompt_helpers.include.c: $(srcdir)/tracer_ompt_helpers.include.c.erb ompt_callbacks.yaml
	SRC_DIR=$(srcdir) $(ERB) -T 1 $< > $@

EXTRA_DIST += \
	gen_ompt.rb \
	ompt_callbacks.yaml \
	tracer_ompt_helpers.include.c.erb

CLEANFILES += \
	tracer_ompt.c \
	tracer_ompt_helpers.include.c

ompprof.sh: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages=["omp"] $< > $@
	chmod a+x $@

CLEANFILES += ompprof.sh

bin_SCRIPTS = \
	tracer_omp.sh \
	babeltrace_omp \
	ompprof.sh

noinst_LTLIBRARIES = libompttracepoints.la

nodist_libompttracepoints_la_SOURCES = \
	$(OMP_PROBES_INCL) \
	$(OMP_PROBES_SRC)

libompttracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libompttracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOMPT.la libOMPInterval.la

nodist_libTracerOMPT_la_SOURCES = \
	$(OMP_PROBES_INCL) \
	tracer_ompt.c

libTracerOMPT_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./modified_include -I../utils -I./
libTracerOMPT_la_CFLAGS = -Wall -Wextra $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerOMPT_la_LDFLAGS = $(LTTNG_UST_LIBS) -avoid-version -module
libTracerOMPT_la_LIBADD = libompttracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ompt
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerOMPT.so $(DESTDIR)$(pkglibdir)/ompt/libompt.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/ompt/libompt.so
	-rmdir $(DESTDIR)$(pkglibdir)/ompt


babeltrace_ompinterval_callbacks.cpp: $(srcdir)/gen_ompinterval_callbacks.rb $(srcdir)/ompinterval_callbacks.cpp.erb omp_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_ompinterval_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb omp_babeltrace_model.yaml ompt.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ ompinterval > $@

babeltrace_ompinterval_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb omp_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ ompinterval > $@

babeltrace_ompinterval_emitter.h: $(top_srcdir)/utils/gen_babeltrace_emitter.rb $(top_srcdir)/xprof/interval_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ ompinterval > $@

babeltrace_ompinterval.h: $(top_srcdir)/xprof/interval.h.erb
	$(ERB) -T 1 namespace="ompinterval" $< > $@

babeltrace_ompinterval.c: $(top_srcdir)/xprof/interval.c.erb
	$(ERB) -T 1 namespace="ompinterval" $< > $@

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

EXTRA_DIST += ompt.h.include

EXTRA_DIST += \
	gen_ompinterval_callbacks.rb \
	ompinterval_callbacks.cpp.erb

CLEANFILES += \
	babeltrace_ompinterval_callbacks.cpp \
	babeltrace_ompinterval_callbacks.h \
	babeltrace_ompinterval_dispatchers.c \
	babeltrace_ompinterval_emitter.h \
	babeltrace_ompinterval.h \
	babeltrace_ompinterval.c \
	xprof_utils.cpp \
	xprof_utils.hpp

BUILT_SOURCES += \
	babeltrace_ompinterval.h \
	babeltrace_ompinterval_callbacks.h \
	babeltrace_ompinterval_emitter.h \
	xprof_utils.hpp

nodist_libOMPInterval_la_SOURCES = \
	babeltrace_ompinterval.c \
	babeltrace_ompinterval.h \
	babeltrace_ompinterval_callbacks.h \
	babeltrace_ompinterval_dispatchers.c \
	babeltrace_ompinterval_callbacks.cpp \
	babeltrace_ompinterval_emitter.h \
	xprof_utils.hpp \
	xprof_utils.cpp

libOMPInterval_la_SOURCES = \
	ompinterval_callbacks.hpp

libOMPInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./modified_include -I./
libOMPInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libOMPInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libOMPInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

omp_library.rb: $(srcdir)/gen_omp_library.rb $(OMP_LIB_GEN) $(OMP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_omp_lib.rb: $(srcdir)/gen_babeltrace_omp_lib.rb $(OMP_LIB_GEN) $(OMP_MODEL) omp_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

OMP_BINDINGS = \
	omp_library.rb \
	babeltrace_omp_lib.rb

EXTRA_DIST += \
	gen_omp_library.rb \
	gen_babeltrace_omp_lib.rb

CLEANFILES += $(OMP_BINDINGS)

data_DATA = \
	$(OMP_BINDINGS)
