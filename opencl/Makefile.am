.DELETE_ON_ERROR:

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./

OPENCL_PROBES_TP = \
                     opencl_tracepoints.tp\
                     opencl_profiling.tp\
                     opencl_source.tp\
                     opencl_dump.tp\
                     opencl_arguments.tp\
                     opencl_build.tp\
                     opencl_devices.tp

OPENCL_PROBES_INCL = \
                     opencl_tracepoints.h\
                     opencl_profiling.h\
                     opencl_source.h\
                     opencl_dump.h\
                     opencl_arguments.h\
                     opencl_build.h\
                     opencl_devices.h

OPENCL_PROBES_SRC = \
                     opencl_tracepoints.c\
                     opencl_profiling.c\
                     opencl_source.c\
                     opencl_dump.c\
                     opencl_arguments.c\
                     opencl_build.c\
                     opencl_devices.c

ML_FILES = $(srcdir)/opencl_meta_parameters.yaml\
           $(srcdir)/supported_extensions.yaml\
           $(srcdir)/supported_enums.yaml

TRACEPOINT_GEN = $(srcdir)/opencl_model.rb\
                 $(srcdir)/opencl_tracepoints.rb\
                 $(srcdir)/opencl_events.yaml

cl.xml.patched: $(srcdir)/cl.xml $(srcdir)/cl.xml.patch
	$(PATCH) $(srcdir)/cl.xml $(srcdir)/cl.xml.patch -o cl.xml.patched

tracer_opencl.h: $(srcdir)/gen_opencl_header.rb
	$(RUBY) $(srcdir)/gen_opencl_header.rb > $@

opencl_model.yaml: $(srcdir)/gen_opencl_model.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/opencl_wrapper_events.yaml $(srcdir)/opencl_tracepoints.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl_model.rb > $@

opencl_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_model.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_babeltrace_model.rb > $@

opencl_dummy_trace.yaml: opencl_babeltrace_model.yaml
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

BUILT_SOURCES = $(OPENCL_PROBES_INCL) babeltrace_cl_callbacks.h clprof_callbacks.h clprof.c

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl.rb > $@

bin_SCRIPTS = tracer_opencl.sh clprof.sh babeltrace_opencl extract_enqueues

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOpenCL.la libCLProf.la

nodist_libTracerOpenCL_la_SOURCES = \
        $(OPENCL_PROBES_INCL) \
        tracer_opencl.h \
	tracer_opencl.c

libTracerOpenCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./
libTracerOpenCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerOpenCL_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerOpenCL_la_LDFLAGS += -Wl,--version-script,$(srcdir)/tracer_opencl.map -version-info 1:0:0
libTracerOpenCL_la_DEPENDS = $(srcdir)/tracer_opencl.map
libTracerOpenCL_la_LIBADD = libtracepoints.la

install-exec-hook::
	mkdir -p $(pkglibdir)/opencl
	$(RM) $(pkglibdir)/opencl/libOpenCL.so.1 $(pkglibdir)/opencl/libOpenCL.so
	ln -s $(libdir)/libTracerOpenCL.so.1.0.0 $(pkglibdir)/opencl/libOpenCL.so.1
	ln -s $(pkglibdir)/opencl/libOpenCL.so.1 $(pkglibdir)/opencl/libOpenCL.so

clprof.c: $(srcdir)/clprof_callbacks.cpp.erb $(srcdir)/clprof_callbacks.h.erb $(srcdir)/clprof.c.erb $(srcdir)/gen_clprof.rb opencl_model.yaml babeltrace_cl_callbacks.h tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_clprof.rb production

clprof_callbacks.cpp clprof_callbacks.h: clprof.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f clprof.c; \
	  $(MAKE) $(AM_MAKEFLAGS) clprof.c; \
	fi

nodist_libCLProf_la_SOURCES = \
	clprof.c \
	clprof_callbacks.cpp \
	clprof_callbacks.h \
	tracer_opencl.h \
	babeltrace_cl_dispatchers.c
libCLProf_la_SOURCES = \
	babeltrace_cl.h

libCLProf_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libCLProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLProf_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceCL.la libDust.la libTestingCLProf.la
install-data-hook::
	$(RM) -r $(DESTDIR)$(tmplibdir)

babeltrace_cl_callbacks.h: $(srcdir)/gen_babeltrace_cl_callbacks.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_babeltrace_cl_callbacks.rb > babeltrace_cl_callbacks.h

babeltrace_cl_dispatchers.c: $(srcdir)/gen_babeltrace_cl_dispatchers.rb opencl_model.yaml opencl_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_babeltrace_cl_dispatchers.rb > babeltrace_cl_dispatchers.c

nodist_libBabeltraceCL_la_SOURCES = \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h
libBabeltraceCL_la_SOURCES = \
	babeltrace_cl.c \
	babeltrace_cl.h
libBabeltraceCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libBabeltraceCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libBabeltraceCL_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

dust.c: $(srcdir)/dust.c.erb $(srcdir)/gen_clprof.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_clprof.rb dust

nodist_libDust_la_SOURCES = \
	dust.c
libDust_la_CFLAGS = $(BABELTRACE2_CFLAGS)
libDust_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

testing_clprof.c: $(srcdir)/clprof_callbacks.cpp.erb $(srcdir)/clprof_callbacks.h.erb $(srcdir)/clprof.c.erb $(srcdir)/gen_clprof.rb opencl_model.yaml babeltrace_cl_callbacks.h tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_clprof.rb testing

testing_clprof_callbacks.cpp testing_clprof_callbacks.h: testing_clprof.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f testing_clprof.c; \
	  $(MAKE) $(AM_MAKEFLAGS) testing_clprof.c; \
	fi

nodist_libTestingCLProf_la_SOURCES = \
	testing_clprof.c \
	testing_clprof_callbacks.cpp \
	testing_clprof_callbacks.h \
	tracer_opencl.h \
	babeltrace_cl_dispatchers.c
libTestingCLProf_la_SOURCES = \
	babeltrace_cl.h

libTestingCLProf_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libTestingCLProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libTestingCLProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libTestingCLProf_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

TEST_FILES = \
	tests/opencl_dummy_trace.dust \
	tests/profiling_normal.dust \
	tests/profiling_inversed.dust \
	tests/profiling_block.dust \
	tests/profiling_fast.dust \
	tests/profiling_interleave_thread.dust \
	tests/profiling_interleave_process.dust \
	tests/profiling_normal_command_queue.dust \
	tests/profiling_with_error.dust \
	tests/profiling_normal_command_queue_created_in_other_thread.dust \
	tests/device_name.dust \
	tests/kernel_name.dust \
	tests/profiling_normal_nd_range_kernel_name.dust \
	tests/API_call.dust \
	tests/trace.dust

TESTS = $(TEST_FILES)

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh
check_DATA = \
	test_wrapper.sh \
	opencl_dummy_trace.yaml

data_DATA = \
	opencl_model.yaml \
	opencl_infos.yaml \
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
	tests/trace.yaml

EXTRA_DIST = \
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
	gen_opencl_custom_probes.rb \
	gen_babeltrace_cl_dispatchers.rb \
	gen_babeltrace_cl_callbacks.rb \
	dust.c.erb \
	gen_clprof.rb \
	clprof_callbacks.cpp.erb \
	babeltrace_opencl.in \
	extract_enqueues \
	clprof.c.erb \
	clprof_callbacks.h.erb \
	test_wrapper.sh

CLEANFILES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_TP) \
	$(OPENCL_PROBES_SRC) \
	tracer_opencl.c \
	tracer_opencl.h \
	cl.xml.patched \
	opencl_model.yaml \
	opencl_babeltrace_model.yaml \
	opencl_dummy_trace.yaml \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h \
	clprof.c \
	clprof_callbacks.cpp \
	clprof_callbacks.h \
	dust.c \
	testing_clprof.c \
	testing_clprof_callbacks.cpp \
	testing_clprof_callbacks.h \
	$(TEST_FILES)
