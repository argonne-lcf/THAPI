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

BUILT_SOURCES = $(OPENCL_PROBES_INCL) babeltrace_cl_callbacks.h clprof_callbacks.h

tracer_opencl.c: $(srcdir)/gen_opencl.rb $(srcdir)/opencl_model.rb $(ML_FILES) cl.xml.patched $(srcdir)/tracer_opencl_helpers.include.c $(OPENCL_PROBES_INCL) tracer_opencl.h
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_opencl.rb > $@

bin_SCRIPTS = tracer_opencl.sh clprof.sh

noinst_LTLIBRARIES = libtracepoints.la

nodist_libtracepoints_la_SOURCES = \
	$(OPENCL_PROBES_INCL) \
	$(OPENCL_PROBES_SRC)

libtracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerOpenCL.la libBabeltraceCL.la libCLProf.la

nodist_libTracerOpenCL_la_SOURCES = \
        $(OPENCL_PROBES_INCL) \
        tracer_opencl.h \
	tracer_opencl.c

libTracerOpenCL_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./
libTracerOpenCL_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror
libTracerOpenCL_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl $(LIBFFI_LIBS)
libTracerOpenCL_la_LDFLAGS += -Wl,--version-script,$(srcdir)/tracer_opencl.map
libTracerOpenCL_la_DEPENDS = $(srcdir)/tracer_opencl.map
libTracerOpenCL_la_LIBADD = libtracepoints.la

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
libBabeltraceCL_la_LDFLAGS = $(BABELTRACE2_LIBS)

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
	clprof.h

libCLProf_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libCLProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCLProf_la_LDFLAGS = -lstdc++ $(BABELTRACE2_LIBS)

CLEANFILES = $(OPENCL_PROBES_INCL) $(OPENCL_PROBES_TP) $(OPENCL_PROBES_SRC) tracer_opencl.c tracer_opencl.h cl.xml.patched opencl_model.yaml babeltrace_cl_dispatchers.c babeltrace_cl_callbacks.h clprof.c clprof_callbacks.cpp clprof_callbacks.h
