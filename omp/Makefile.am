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
	$(srcdir)/../utils/yaml_ast_lttng.rb \
	$(srcdir)/../utils/meta_parameters.rb \
	$(srcdir)/../utils/LTTng.rb \
	$(OMP_EXTRACTED)

BTX_OMP_GENERATED = \
	btx_filter_omp/metababel/metababel.h \
	btx_filter_omp/metababel/btx_component.h \
	btx_filter_omp/metababel/btx_component.c \
	btx_filter_omp/metababel/btx_upstream.h \
	btx_filter_omp/metababel/btx_upstream.c \
	btx_filter_omp/metababel/btx_downstream.h \
	btx_filter_omp/metababel/btx_downstream.c \
	btx_filter_omp/btx_main.c

OMP_LIB_GEN = \
	$(srcdir)/../utils/gen_library_base.rb \
	$(srcdir)/gen_omp_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += \
	$(OMP_LIB_GEN) \
	$(srcdir)/ompt_meta_parameters.yaml

omp_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_omp_model.rb $(OMP_LIB_GEN) $(OMP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

btx_omp_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb omp_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_OMP_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_omp_model.yaml
	$(METABABEL) -u btx_omp_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_omp -p ompinterval -c interval -i ompt.h.include

CLEANFILES += \
	omp_babeltrace_model.yaml \
	btx_omp_model.yaml

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

bin_SCRIPTS = \
	tracer_omp.sh

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

libTracerOMPT_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I./modified_include -I../utils -I./
libTracerOMPT_la_CFLAGS = -Wall -Wextra $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerOMPT_la_LDFLAGS = $(LTTNG_UST_LIBS) -avoid-version -module ../sampling/libThapiSampling.la
libTracerOMPT_la_LIBADD = libompttracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ompt
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerOMPT.so $(DESTDIR)$(pkglibdir)/ompt/libompt.so
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libOMPInterval.so $(DESTDIR)$(pkglibdir)/bt2/libOMPInterval.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/ompt/libompt.so
	-rmdir $(DESTDIR)$(pkglibdir)/ompt
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libOMPInterval.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

EXTRA_DIST += ompt.h.include

CLEANFILES += \
	$(BTX_OMP_GENERATED) \
	xprof_utils.cpp \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_OMP_GENERATED) \
	xprof_utils.hpp

nodist_libOMPInterval_la_SOURCES = \
	$(BTX_OMP_GENERATED) \
	xprof_utils.hpp \
	xprof_utils.cpp

libOMPInterval_la_SOURCES = \
	btx_ompinterval_callbacks.cpp \
	$(top_srcdir)/utils/include/magic_enum.hpp

libOMPInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./modified_include -I./ -I./btx_filter_omp
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

# Tests

TRACE_COMMON = \
	tests/data_op.thapi_text_pretty \
	tests/target.thapi_text_pretty \
	tests/submit.thapi_text_pretty \
	tests/data_op_emi.thapi_text_pretty \
	tests/target_emi.thapi_text_pretty \
	tests/submit_emi.thapi_text_pretty

BTX_OMP_GENERATED_SOURCE_TEST = \
	btx_source_omp_test/metababel/metababel.h \
	btx_source_omp_test/metababel/btx_component.h \
	btx_source_omp_test/metababel/btx_component.c \
	btx_source_omp_test/metababel/btx_downstream.h \
	btx_source_omp_test/metababel/btx_downstream.c \
	btx_source_omp_test/btx_main.c

$(BTX_OMP_GENERATED_SOURCE_TEST) &: btx_omp_model.yaml
	$(METABABEL) -d btx_omp_model.yaml -t SOURCE  -o btx_source_omp_test -p omptest -c test -i ompt.h.include

noinst_LTLIBRARIES += libtestompsource.la
libtestompsource_la_SOURCES = $(BTX_OMP_GENERATED_SOURCE_TEST)
libtestompsource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./modified_include -I$(srcdir) -I./btx_source_omp_test/
libtestompsource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

TRACE_OUT = $(TRACE_COMMON:.thapi_text_pretty=.bt_text_pretty)

TESTS = \
	$(TRACE_COMMON)

TEST_EXTENSIONS = .thapi_text_pretty
THAPI_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_TEXT_PRETTY_LOG_FLAGS = omp omp ./btx_source_omp_test/ .libs/libtestompsource.a

check_DATA = \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

EXTRA_DIST += \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

CLEANFILES += \
	$(TRACE_COMMON) \
	$(BTX_OMP_GENERATED_SOURCE_TEST)
