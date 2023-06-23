.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

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

opencl_model.yaml: $(srcdir)/gen_opencl_model.rb $(OPENCL_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

opencl_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_cl_model.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

data_DATA = \
	opencl_model.yaml \
	babeltrace_opencl_lib.rb

CLEANFILES += \
	opencl_model.yaml \
	opencl_babeltrace_model.yaml

EXTRA_DIST += \
	$(TRACEPOINT_GEN) \
	$(ML_FILES) \
	gen_opencl_model.rb \
	babeltrace_opencl_lib.rb \
	gen_babeltrace_cl_model.rb

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

clprof.sh: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages="cl" $< > $@
	chmod a+x $@

CLEANFILES += clprof.sh

EXTRA_DIST += opencl_infos.yaml

data_DATA += \
	opencl_infos.yaml

bin_SCRIPTS = \
	tracer_opencl.sh \
	clprof.sh \
	extract_enqueues

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(OPENCL_MODEL) $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) $(OPENCL_STATIC_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += tracer_opencl.c

EXTRA_DIST += \
	tracer_opencl_helpers.include.c \
	gen_opencl.rb

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC) \
	$(OPENCL_STATIC_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOpenCL.la libCLInterval.la

nodist_libTracerOpenCL_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_STATIC_PROBES_INCL) \
	tracer_opencl.h \
	tracer_opencl.c

libTracerOpenCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libTracerOpenCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerOpenCL_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerOpenCL_la_LDFLAGS += -Wl,--version-script,$(srcdir)/tracer_opencl.map -version-info 1:0:0
libTracerOpenCL_la_DEPENDS = $(srcdir)/tracer_opencl.map
libTracerOpenCL_la_LIBADD = libtracepoints.la

thapi_session: libTracerOpenCL.la
	$(top_srcdir)/utils/lttng_dummy_trace.sh ./.libs/libTracerOpenCL.so lttng_ust_opencl:\*,lttng_ust_opencl_build:\*,lttng_ust_opencl_arguments:\*,lttng_ust_opencl_dump:\*,lttng_ust_opencl_profiling:\*,lttng_ust_opencl_source:\*,lttng_ust_opencl_devices:\*

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/opencl
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerOpenCL.so.1.0.0 $(DESTDIR)$(pkglibdir)/opencl/libOpenCL.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/opencl/libOpenCL.so.1 $(DESTDIR)$(pkglibdir)/opencl/libOpenCL.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/opencl/libOpenCL.so.1 $(DESTDIR)$(pkglibdir)/opencl/libOpenCL.so
	-rmdir $(DESTDIR)$(pkglibdir)/opencl

# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceCL.la

install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

babeltrace_cl_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb opencl_babeltrace_model.yaml cl.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ cl > $@

babeltrace_cl_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ cl > $@

EXTRA_DIST += cl.h.include

CLEANFILES += \
	babeltrace_cl_callbacks.h \
	babeltrace_cl_dispatchers.c

BUILT_SOURCES += \
	babeltrace_cl_callbacks.h

nodist_libBabeltraceCL_la_SOURCES = \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h

libBabeltraceCL_la_SOURCES = \
	babeltrace_cl.c \
	babeltrace_cl.h

libBabeltraceCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libBabeltraceCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libBabeltraceCL_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

babeltrace_clinterval_callbacks.cpp: $(srcdir)/gen_clinterval_callbacks.rb $(srcdir)/clinterval_callbacks.cpp.erb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_clinterval_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb opencl_babeltrace_model.yaml cl.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ clinterval > $@

babeltrace_clinterval_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ clinterval > $@

babeltrace_clinterval_emitter.h: $(top_srcdir)/utils/gen_babeltrace_emitter.rb $(top_srcdir)/xprof/interval_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ clinterval > $@

babeltrace_clinterval.h: $(top_srcdir)/xprof/interval.h.erb
	$(ERB) namespace="clinterval" $< > $@

babeltrace_clinterval.c: $(top_srcdir)/xprof/interval.c.erb
	$(ERB) namespace="clinterval" $< > $@

# opencl/Makefile.am:248: warning: source file '$(top_srcdir)/utils/xprof_utils.cpp' is in a subdirectory,
# opencl/Makefile.am:248: but option 'subdir-objects' is disabled
xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

EXTRA_DIST += \
	clinterval_callbacks.cpp.erb \
	gen_clinterval_callbacks.rb

CLEANFILES += \
	babeltrace_clinterval_callbacks.cpp \
	babeltrace_clinterval_callbacks.h \
	babeltrace_clinterval_dispatchers.c \
	babeltrace_clinterval_emitter.h \
	babeltrace_clinterval.h \
	babeltrace_clinterval.c \
	xprof_utils.cpp \
	xprof_utils.hpp

BUILT_SOURCES += \
	babeltrace_clinterval_callbacks.h \
	babeltrace_clinterval.h \
	babeltrace_clinterval_emitter.h \
	xprof_utils.hpp

nodist_libCLInterval_la_SOURCES = \
	babeltrace_clinterval.c \
	babeltrace_clinterval.h \
	babeltrace_clinterval_callbacks.h \
	babeltrace_clinterval_dispatchers.c \
	babeltrace_clinterval_callbacks.cpp \
	babeltrace_clinterval_emitter.h \
	tracer_opencl.h \
	xprof_utils.hpp \
	xprof_utils.cpp

libCLInterval_la_SOURCES = \
	clinterval_callbacks.hpp

libCLInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libCLInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCLInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCLInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

tests/.tag:
	mkdir -p tests
	touch tests/.tag

clean-local:
	$(RM) -r tests

CLEANFILES += tests/.tag

tests/opencl_dummy_trace.yaml: $(top_srcdir)/utils/gen_dummy_trace.rb opencl_babeltrace_model.yaml tests/.tag
	$(RUBY) $< opencl_babeltrace_model.yaml > $@

# To run test we need to catageory of file:
# - One are the `yaml` file who containt the traces
# - The other is the `dust` file who containt the babeltrace processing graph
#   The dust file will use the `yaml` file.

# Test by themself are splited into two type:
# - The one who are common to each backend.
# - The specific for this backend

# We generate lot of file using:
#https://www.gnu.org/software/make/manual/html_node/Static-Usage.html

TRACE_COMMON = \
	tests/interval_profiling_block.yaml \
	tests/interval_profiling_normal.yaml

$(TRACE_COMMON): tests/%.yaml: $(top_srcdir)/utils/tests_template/%.yaml.erb tests/.tag
	$(ERB) be="opencl" $< > $@

DUST_COMMON = $(TRACE_COMMON:.yaml=.dust)

$(DUST_COMMON): tests/interval_%.dust: $(top_srcdir)/utils/tests_template/interval_common.dust.erb tests/.tag
	$(ERB) be="opencl" interval="clinterval" namespace="$@" $< > $@

TRACE_SPECIFC = \
	tests/profiling_inversed.yaml \
	tests/profiling_fast.yaml \
	tests/profiling_interleave_thread.yaml \
	tests/profiling_interleave_process.yaml \
	tests/profiling_normal_command_queue.yaml \
	tests/profiling_with_error.yaml \
	tests/profiling_normal_command_queue_created_in_other_thread.yaml \
	tests/device_name.yaml \
	tests/kernel_name.yaml \
	tests/profiling_normal_nd_range_kernel_name.yaml \
	tests/API_call.yaml \
	tests/trace.yaml \
	tests/interval_profiling_inversed.yaml \
	tests/interval_profiling_fast.yaml \
	tests/interval_profiling_interleave_thread.yaml \
	tests/interval_profiling_interleave_process.yaml \
	tests/interval_profiling_normal_command_queue.yaml \
	tests/interval_profiling_with_error.yaml \
	tests/interval_profiling_normal_command_queue_created_in_other_thread.yaml \
	tests/interval_profiling_normal_nd_range_kernel_name.yaml

DUST_SPECIFIC = \
	tests/interval_profiling_inversed.dust \
	tests/interval_profiling_fast.dust \
	tests/interval_profiling_interleave_thread.dust \
	tests/interval_profiling_interleave_process.dust \
	tests/interval_profiling_normal_command_queue.dust \
	tests/interval_profiling_with_error.dust \
	tests/interval_profiling_normal_command_queue_created_in_other_thread.dust \
	tests/interval_profiling_normal_nd_range_kernel_name.dust

$(DUST_SPECIFIC): tests/interval_%.dust: tests/interval.dust.erb
	$(ERB) namespace="$@" $< > $@

TESTS = \
	tests/opencl_dummy_trace.dust \
	tests/trace.dust \
	tests/test_interval.dust \
	$(DUST_COMMON) \
	$(DUST_SPECIFIC)

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh

check_DATA = \
	test_wrapper.sh \
	tests/opencl_dummy_trace.yaml \
	$(TRACE_COMMON) \
	$(TRACE_SPECIFC)

EXTRA_DIST += \
	extract_enqueues \
	$(TRACE_SPECIFC) \
	tests/trace.dust \
	tests/test_interval.dust \
	tests/opencl_dummy_trace.dust \
	tests/interval.dust.erb

CLEANFILES += \
	tests/opencl_dummy_trace.yaml \
	$(DUST_COMMON) \
	$(DUST_SPECIFIC) \
	$(TRACE_COMMON) 
