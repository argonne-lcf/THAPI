.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

cl.xml.patched: $(srcdir)/cl.xml $(srcdir)/cl.xml.patch
	$(PATCH) $(srcdir)/cl.xml $(srcdir)/cl.xml.patch -o cl.xml.patched

CLEANFILES = \
	cl.xml.patched

EXTRA_DIST = \
	cl.xml \
	cl.xml.patch

tracer_opencl.h: $(srcdir)/gen_opencl_header.rb
	$(RUBY) $< > $@

BUILT_SOURCES = tracer_opencl.h

CLEANFILES += tracer_opencl.h

EXTRA_DIST += gen_opencl_header.rb

ML_FILES = \
	$(srcdir)/opencl_meta_parameters.yaml \
	$(srcdir)/supported_extensions.yaml \
	$(srcdir)/supported_enums.yaml

TRACEPOINT_GEN = \
	$(srcdir)/opencl_model.rb \
	$(srcdir)/opencl_tracepoints.rb \
	$(srcdir)/opencl_wrapper_events.yaml \
	$(srcdir)/opencl_events.yaml

OPENCL_MODEL = \
	$(TRACEPOINT_GEN) \
	$(ML_FILES) \
	cl.xml.patched

BTX_OPENCL_GENERATED = \
	btx_filter_opencl/metababel/metababel.h \
	btx_filter_opencl/metababel/btx_component.h \
	btx_filter_opencl/metababel/btx_component.c \
	btx_filter_opencl/metababel/btx_upstream.h \
	btx_filter_opencl/metababel/btx_upstream.c \
	btx_filter_opencl/metababel/btx_downstream.h \
	btx_filter_opencl/metababel/btx_downstream.c \
	btx_filter_opencl/btx_main.c

opencl_model.yaml: $(srcdir)/gen_opencl_model.rb $(OPENCL_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

opencl_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_cl_model.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

btx_cl_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_OPENCL_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_clmatching_model.yaml btx_cl_model.yaml
	$(METABABEL) -u btx_cl_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_opencl -p clinterval -c interval --matching $(srcdir)/btx_clmatching_model.yaml -i cl.h.include

data_DATA = \
	opencl_model.yaml \
	babeltrace_opencl_lib.rb

CLEANFILES += \
	opencl_model.yaml \
	opencl_babeltrace_model.yaml \
	btx_cl_model.yaml \
	$(BTX_OPENCL_GENERATED)

EXTRA_DIST += \
	$(TRACEPOINT_GEN) \
	$(ML_FILES) \
	gen_opencl_model.rb \
	babeltrace_opencl_lib.rb \
	gen_babeltrace_cl_model.rb \
	$(top_srcdir)/utils/babeltrace_to_metababel_model.rb \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	btx_clmatching_model.yaml

OPENCL_PROBES = \
	opencl_tracepoints

OPENCL_PROBES_TP = $(OPENCL_PROBES:=.tp)

OPENCL_PROBES_INCL = $(OPENCL_PROBES:=.h)

OPENCL_PROBES_SRC = $(OPENCL_PROBES:=.c)

OPENCL_STATIC_PROBES = \
	opencl_profiling \
	opencl_source \
	opencl_dump \
	opencl_arguments \
	opencl_build \
	opencl_devices

OPENCL_STATIC_PROBES_TP = $(OPENCL_STATIC_PROBES:=.tp)

OPENCL_STATIC_PROBES_INCL = $(OPENCL_STATIC_PROBES:=.h)

OPENCL_STATIC_PROBES_SRC = $(OPENCL_STATIC_PROBES:=.c)

opencl_tracepoints.tp: $(srcdir)/gen_opencl_probes.rb $(OPENCL_MODEL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

$(OPENCL_STATIC_PROBES_TP): %.tp: $(srcdir)/gen_opencl_custom_probes.rb $(OPENCL_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< lttng_ust_$* > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_TP) \
	$(OPENCL_PROBES_SRC) \
	$(OPENCL_STATIC_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_TP) \
	$(OPENCL_STATIC_PROBES_SRC)

EXTRA_DIST += \
	include \
	tracer_opencl.map \
	gen_opencl_probes.rb \
	gen_opencl_custom_probes.rb

BUILT_SOURCES += \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_INCL)

EXTRA_DIST += opencl_infos.yaml

data_DATA += \
	opencl_infos.yaml

bin_SCRIPTS = \
	tracer_opencl.sh \
	extract_enqueues

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(OPENCL_MODEL) $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) $(OPENCL_STATIC_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += tracer_opencl.c

EXTRA_DIST += \
	tracer_opencl_helpers.include.c \
	gen_opencl.rb

noinst_LTLIBRARIES = libOpenCLtracepoints.la

nodist_libOpenCLtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC) \
	$(OPENCL_STATIC_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_SRC)

libOpenCLtracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libOpenCLtracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libOpenCLtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

opencldir = $(pkglibdir)/opencl
opencl_LTLIBRARIES = libOpenCL.la

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libCLInterval.la

nodist_libOpenCL_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_INCL) \
	tracer_opencl.h \
	tracer_opencl.c

libOpenCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libOpenCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libOpenCL_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libOpenCL_la_LDFLAGS += -Wl,--version-script,$(srcdir)/tracer_opencl.map -version-info 1:0:0
libOpenCL_la_DEPENDS = $(srcdir)/tracer_opencl.map
libOpenCL_la_LIBADD = libOpenCLtracepoints.la

# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp

install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

EXTRA_DIST += cl.h.include

# opencl/Makefile.am:248: warning: source file '$(top_srcdir)/utils/xprof_utils.cpp' is in a subdirectory,
# opencl/Makefile.am:248: but option 'subdir-objects' is disabled
xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
	$(BTX_OPENCL_GENERATED) \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_OPENCL_GENERATED) \
	xprof_utils.hpp

nodist_libCLInterval_la_SOURCES = \
	$(BTX_OPENCL_GENERATED) \
	xprof_utils.hpp

libCLInterval_la_SOURCES = \
	btx_clinterval_callbacks.cpp

libCLInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_opencl -DCL_TARGET_OPENCL_VERSION=300
libCLInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCLInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCLInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

TRACE_COMMON = \
	tests/api_call.thapi_text_pretty \
	tests/nd_range_kernel.thapi_text_pretty \
	tests/enqueue_read_buffer_fast.thapi_text_pretty \
	tests/interleave_thread.thapi_text_pretty \
	tests/interleave_process.thapi_text_pretty \
	tests/results_first.thapi_text_pretty \
	tests/command_queue.thapi_text_pretty \
	tests/command_queue_other_thread.thapi_text_pretty \
	tests/with_error.thapi_text_pretty

BTX_OPENCL_GENERATED_SOURCE_TEST = \
	btx_source_cl_test/metababel/metababel.h \
	btx_source_cl_test/metababel/btx_component.h \
	btx_source_cl_test/metababel/btx_component.c \
	btx_source_cl_test/metababel/btx_downstream.h \
	btx_source_cl_test/metababel/btx_downstream.c \
	btx_source_cl_test/btx_main.c

$(BTX_OPENCL_GENERATED_SOURCE_TEST) &: btx_cl_model.yaml
	$(METABABEL) -d btx_cl_model.yaml -t SOURCE -o btx_source_cl_test -p cltest -c test -i cl.h.include

noinst_LTLIBRARIES += libtestclsource.la
nodist_libtestclsource_la_SOURCES = $(BTX_OPENCL_GENERATED_SOURCE_TEST)
libtestclsource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_cl_test/
libtestclsource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

TRACE_OUT = $(TRACE_COMMON:.thapi_text_pretty=.bt_text_pretty)

TESTS = \
	$(TRACE_COMMON)

TEST_EXTENSIONS = .thapi_text_pretty
THAPI_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_TEXT_PRETTY_LOG_FLAGS = opencl cl ./btx_source_cl_test/ .libs/libtestclsource.a

check_DATA = \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

EXTRA_DIST += \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

CLEANFILES += \
	$(TRACE_COMMON) \
	$(BTX_OPENCL_GENERATED_SOURCE_TEST)
