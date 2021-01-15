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

cldispatch.sh: $(top_builddir)/utils/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@

BUILT_SOURCES = $(OPENCL_PROBES_INCL) babeltrace_cl_callbacks.h cldispatch_callbacks.h cldispatch.c

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl.rb > $@

bin_SCRIPTS = tracer_opencl.sh cldispatch.sh babeltrace_opencl extract_enqueues

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOpenCL.la libcldispatch.la

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

cldispatch.c: $(srcdir)/cldispatch_callbacks.cpp.erb $(srcdir)/cldispatch_callbacks.h.erb $(srcdir)/cldispatch_callbacks.h.erb $(srcdir)/cldispatch.c.erb $(srcdir)/cldispatch.h.erb $(srcdir)/gen_cldispatch.rb opencl_model.yaml babeltrace_cl_callbacks.h tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cldispatch.rb production

cldispatch_callbacks.cpp cldispatch_callbacks.h: cldispatch.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f cldispatch.c; \
	  $(MAKE) $(AM_MAKEFLAGS) cldispatch.c; \
	fi


# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceCL.la libDust.la #libTestingcldispatch.la
install-data-hook::
	$(RM) -r $(DESTDIR)$(tmplibdir)

babeltrace_cl_callbacks.h: $(srcdir)/gen_babeltrace_cl_callbacks.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_babeltrace_cl_callbacks.rb > babeltrace_cl_callbacks.h

babeltrace_cl_dispatchers.c: $(srcdir)/gen_babeltrace_cl_dispatchers.rb opencl_model.yaml
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

dust.c: $(srcdir)/dust.c.erb $(srcdir)/gen_cldispatch.rb opencl_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cldispatch.rb dust

nodist_libDust_la_SOURCES = \
	dust.c
libDust_la_CFLAGS = $(BABELTRACE2_CFLAGS)
libDust_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

testing_cldispatch.c: $(srcdir)/cldispatch_callbacks.cpp.erb $(srcdir)/cldispatch_callbacks.h.erb $(srcdir)/cldispatch.c.erb $(srcdir)/gen_cldispatch.rb opencl_model.yaml babeltrace_cl_callbacks.h tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cldispatch.rb testing

testing_cldispatch_callbacks.cpp testing_cldispatch_callbacks.h: testing_cldispatch.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f testing_cldispatch.c; \
	  $(MAKE) $(AM_MAKEFLAGS) testing_cldispatch.c; \
	fi

nodist_libcldispatch_la_SOURCES = \
	clprof.c \
	cldispatch.h \
	cldispatch.c \
	cldispatch_callbacks.cpp \
	cldispatch_callbacks.h \
	testing_cldispatch.h \
	testing_cldispatch.c \
	testing_cldispatch_callbacks.cpp \
	testing_cldispatch_callbacks.h \
	tracer_opencl.h \
	babeltrace_cl_dispatchers.c
libcldispatch_la_SOURCES = \
	babeltrace_cl.h

libcldispatch_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libcldispatch_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libcldispatch_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libcldispatch_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

TEST_FILES = \
	profiling_normal.dust \
	profiling_inversed.dust \
	profiling_block.dust \
	profiling_fast.dust \
	profiling_interleave_thread.dust \
	profiling_interleave_process.dust \
	profiling_normal_command_queue.dust \
	profiling_with_error.dust \
	profiling_normal_command_queue_created_in_other_thread.dust \
	device_name.dust \
	kernel_name.dust \
	profiling_normal_nd_range_kernel_name.dust \
	API_call.dust

TESTS = $(TEST_FILES)
	
TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = $(srcdir)/test_wrapper.sh

data_DATA = \
	opencl_model.yaml \
	opencl_infos.yaml

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
	gen_cldispatch.rb \
	cldispatch.h.erb \
	cldispatch_callbacks.cpp.erb \
	cldispatch_callbacks.h.erb \
	babeltrace_opencl.in \
	extract_enqueues \
	cldispatch.c.erb \
	cldispatch_callbacks.h.erb \
	test_wrapper.sh

CLEANFILES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_TP) \
	$(OPENCL_PROBES_SRC) \
    clprof.c \
    tracer_opencl.c \
	tracer_opencl.h \
	cl.xml.patched \
	opencl_model.yaml \
	babeltrace_cl_dispatchers.c \
	babeltrace_cl_callbacks.h \
	cldispatch.c \
	cldispatch.h \
    cldispatch_callbacks.cpp \
	cldispatch_callbacks.h \
	dust.c \
	testing_cldispatch.c \
	testing_cldispatch.h \
	testing_cldispatch_callbacks.cpp \
	testing_cldispatch_callbacks.h \
	$(TEST_FILES)
