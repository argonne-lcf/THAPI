.DELETE_ON_ERROR:

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

OPENCL_PROBES_TP = \
	opencl_tracepoints.tp \
	opencl_profiling.tp \
	opencl_source.tp \
	opencl_dump.tp \
	opencl_arguments.tp \
	opencl_build.tp \
	opencl_devices.tp

OPENCL_PROBES_INCL = \
	opencl_tracepoints.h \
	opencl_profiling.h \
	opencl_source.h \
	opencl_dump.h \
	opencl_arguments.h \
	opencl_build.h \
	opencl_devices.h

OPENCL_PROBES_SRC = \
	opencl_tracepoints.c \
	opencl_profiling.c \
	opencl_source.c \
	opencl_dump.c \
	opencl_arguments.c \
	opencl_build.c \
	opencl_devices.c

ML_FILES = \
	$(srcdir)/opencl_meta_parameters.yaml \
	$(srcdir)/supported_extensions.yaml \
	$(srcdir)/supported_enums.yaml

TRACEPOINT_GEN = \
	$(srcdir)/opencl_model.rb \
	$(srcdir)/opencl_tracepoints.rb \
	$(srcdir)/opencl_events.yaml

cl.xml.patched: $(srcdir)/cl.xml $(srcdir)/cl.xml.patch
	$(PATCH) $(srcdir)/cl.xml $(srcdir)/cl.xml.patch -o cl.xml.patched

tracer_opencl.h: $(srcdir)/gen_opencl_header.rb
	$(RUBY) $(srcdir)/gen_opencl_header.rb > $@

opencl_model.yaml: $(srcdir)/gen_opencl_model.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/opencl_wrapper_events.yaml $(srcdir)/opencl_tracepoints.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_model.rb > $@

opencl_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_cl_model.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_babeltrace_cl_model.rb > $@

tests/opencl_dummy_trace.yaml: opencl_babeltrace_model.yaml
	mkdir -p tests
	$(RUBY) $(top_srcdir)/utils/gen_dummy_trace.rb opencl_babeltrace_model.yaml > $@

opencl_tracepoints.tp: $(srcdir)/gen_opencl_probes.rb $(srcdir)/opencl_model.rb tracer_opencl.h $(ML_FILES) cl.xml.patched $(srcdir)/opencl_tracepoints.rb $(srcdir)/opencl_wrapper_events.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_probes.rb > $@

opencl_build.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_build > $@

opencl_arguments.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_arguments > $@

opencl_dump.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_dump > $@

opencl_profiling.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_profiling > $@

opencl_source.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_source > $@

opencl_devices.tp: $(srcdir)/gen_opencl_custom_probes.rb $(TRACEPOINT_GEN)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_custom_probes.rb lttng_ust_opencl_devices > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

clprof.sh: $(top_builddir)/utils/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@

BUILT_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	babeltrace_cl_callbacks.h \
	babeltrace_clinterval.h \
	babeltrace_clinterval_callbacks.h \
	babeltrace_clinterval.c

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl.rb > $@

bin_SCRIPTS = tracer_opencl.sh clprof.sh babeltrace_opencl extract_enqueues

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOpenCL.la libCLInterval.la

nodist_libTracerOpenCL_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	tracer_opencl.h \
	tracer_opencl.c

libTracerOpenCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libTracerOpenCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
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

babeltrace_cl_dispatchers.c:  $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ cl > $@

EXTRA_DIST = cl.h.include

nodist_libBabeltraceCL_la_SOURCES = \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h
libBabeltraceCL_la_SOURCES = \
	babeltrace_cl.c \
	babeltrace_cl.h

libBabeltraceCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libBabeltraceCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libBabeltraceCL_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

babeltrace_clinterval_callbacks.cpp: $(srcdir)/clinterval_callbacks.cpp.erb $(srcdir)/gen_clinterval_callbacks.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_clinterval_callbacks.rb  > $@

babeltrace_clinterval_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb opencl_babeltrace_model.yaml cl.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ clinterval > $@

babeltrace_clinterval_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ clinterval > $@

babeltrace_clinterval.h: $(top_srcdir)/xprof/interval.h.erb
	$(ERB) namespace="clinterval" $(top_srcdir)/xprof/interval.h.erb > $@

babeltrace_clinterval.c: $(top_srcdir)/xprof/interval.c.erb
	$(ERB) namespace="clinterval"  $(top_srcdir)/xprof/interval.c.erb > $@

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

BUILT_SOURCES += \
	babeltrace_clinterval.h \
	babeltrace_cl.h \
	babeltrace_clinterval_callbacks.h \
	xprof_utils.hpp

nodist_libCLInterval_la_SOURCES = \
	babeltrace_clinterval.c \
	babeltrace_clinterval.h \
	babeltrace_clinterval_callbacks.h \
	babeltrace_clinterval_dispatchers.c \
	babeltrace_clinterval_callbacks.cpp \
	tracer_opencl.h \
	xprof_utils.hpp \
	xprof_utils.cpp

libCLInterval_la_SOURCES = \
	clinterval_callbacks.hpp

libCLInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libCLInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

TESTS_COMMON_TEMPLATE = \
	tests/interval_profiling_block.yaml

$(TESTS_COMMON_TEMPLATE): tests/%.yaml: $(top_srcdir)/utils/tests_template/%.yaml.erb
	$(ERB) profiled_call="clEnqueueReadBuffer" $< > $@

#https://www.gnu.org/software/make/manual/html_node/Static-Usage.html
INTERVAL_DUST_FILES = \
	tests/interval_profiling_normal.dust \
	tests/interval_profiling_inversed.dust \
	tests/interval_profiling_block.dust \
	tests/interval_profiling_fast.dust \
	tests/interval_profiling_interleave_thread.dust \
	tests/interval_profiling_interleave_process.dust \
	tests/interval_profiling_normal_command_queue.dust \
	tests/interval_profiling_with_error.dust \
	tests/interval_profiling_normal_command_queue_created_in_other_thread.dust \
	tests/interval_profiling_normal_nd_range_kernel_name.dust

$(INTERVAL_DUST_FILES): tests/interval_%.dust: tests/interval.dust.erb
	$(ERB) namespace="$@" $(srcdir)/tests/interval.dust.erb > $@

nodist_libCLInterval_la_SOURCES+= \
	$(INTERVAL_DUST_FILES)

TRACE_FILES = \
	tests/profiling_normal.yaml \
	tests/profiling_inversed.yaml \
	tests/profiling_block.yaml \
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
	tests/interval_profiling_normal.yaml \
	tests/interval_profiling_inversed.yaml \
	tests/interval_profiling_block.yaml \
	tests/interval_profiling_fast.yaml \
	tests/interval_profiling_interleave_thread.yaml \
	tests/interval_profiling_interleave_process.yaml \
	tests/interval_profiling_normal_command_queue.yaml \
	tests/interval_profiling_with_error.yaml \
	tests/interval_profiling_normal_command_queue_created_in_other_thread.yaml \
	tests/interval_profiling_normal_nd_range_kernel_name.yaml

TESTS = \
	tests/opencl_dummy_trace.dust \
	tests/trace.dust \
	$(INTERVAL_DUST_FILES)

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh

check_DATA = \
	test_wrapper.sh \
	tests/opencl_dummy_trace.yaml \
	$(TRACE_FILES)

data_DATA = \
	opencl_model.yaml \
	opencl_infos.yaml

EXTRA_DIST += \
	cl.xml \
	cl.xml.patch \
	$(ML_FILES) \
	opencl_infos.yaml \
	opencl_wrapper_events.yaml \
	$(TRACEPOINT_GEN) \
	include \
	tracer_opencl.map \
	tracer_opencl_helpers.include.c \
	gen_opencl.rb \
	gen_opencl_probes.rb \
	gen_opencl_header.rb \
	gen_opencl_model.rb \
	gen_babeltrace_cl_model.rb \
	gen_opencl_custom_probes.rb \
	babeltrace_opencl.in \
	extract_enqueues \
	gen_clinterval_callbacks.rb \
	clinterval_callbacks.cpp.erb \
	clinterval_callbacks.hpp \
	xprof_utils.hpp \
	xprof_utils.cpp \
	$(TRACE_FILES) \
	tests/trace.dust \
	tests/opencl_dummy_trace.dust \
	tests/interval.dust.erb 

CLEANFILES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_TP) \
	$(OPENCL_PROBES_SRC) \
	tracer_opencl.c \
	tracer_opencl.h \
	cl.xml.patched \
	opencl_model.yaml \
	opencl_babeltrace_model.yaml \
	tests/opencl_dummy_trace.yaml \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h \
	clprof.sh \
	xprof_utils.cpp \
	xprof_utils.hpp \
	$(nodist_libCLInterval_la_SOURCES) \
	$(CLPROF_DUST_FILES) \
	$(INTERVAL_DUST_FILES)
