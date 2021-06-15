.DELETE_ON_ERROR:

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

CUDA_PROBES_TP = \
	cuda_tracepoints.tp \
	cuda_exports_tracepoints.tp

CUDA_PROBES_INCL = \
	cuda_tracepoints.h \
	cuda_exports_tracepoints.h

CUDA_PROBES_SRC = \
	cuda_tracepoints.c \
	cuda_exports_tracepoints.c

CUDA_STATIC_PROBES_TP = \
	cuda_args.tp \
	cuda_profiling.tp \
	cuda_properties.tp

CUDA_STATIC_PROBES_INCL = \
	cuda_args.h \
	cuda_profiling.h \
	cuda_properties.h

CUDA_STATIC_PROBES_SRC = \
	cuda_args.c \
	cuda_profiling.c \
	cuda_properties.c

CUDART_PROBES_TP = \
	cudart_tracepoints.tp

CUDART_PROBES_INCL = \
	cudart_tracepoints.h

CUDART_PROBES_SRC = \
	cudart_tracepoints.c

CLEANFILES = \
	$(CUDA_PROBES_TP) \
	$(CUDA_PROBES_INCL) \
	$(CUDA_PROBES_SRC) \
	$(CUDA_STATIC_PROBES_TP) \
	$(CUDA_STATIC_PROBES_INCL) \
	$(CUDA_STATIC_PROBES_SRC) \
	$(CUDART_PROBES_TP) \
	$(CUDART_PROBES_INCL) \
	$(CUDART_PROBES_SRC)

ML_CUDA_FILES = \
	$(srcdir)/cuda_meta_parameters.yaml \
	$(srcdir)/cuda_exports_meta_parameters.yaml

ML_CUDART_FILES = \
	$(srcdir)/cudart_meta_parameters.yaml

EXTRA_DIST = $(ML_CUDA_FILES) $(ML_CUDART_FILES)

CUDA_EXTRACT = \
	$(srcdir)/extract_base.rb

EXTRA_DIST += extract_base.rb

CUDA_EXTRACTED = \
	cuda_api.yaml \
	cuda_exports_api.yaml

CUDART_EXTRACTED = \
	cudart_api.yaml

CLEANFILES += $(CUDA_EXTRACTED) $(CUDART_EXTRACTED)

CUDA_MODEL = \
	$(srcdir)/cuda_model.rb \
	$(srcdir)/gen_cuda_library_base.rb \
	$(srcdir)/gen_probe_base.rb \
	$(ML_CUDA_FILES) \
	$(srcdir)/cuda_events.yaml \
	$(CUDA_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb \
	$(srcdir)/cuda.h.include \
	cuda_exports.h.include

CUDART_MODEL = \
	$(srcdir)/cudart_model.rb \
	$(ML_CUDART_FILES) \
	$(CUDART_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb

EXTRA_DIST += \
	cuda_model.rb \
	cuda_events.yaml \
	cudart_model.rb

CUDA_EXTRA_HDR = \
	$(srcdir)/include/builtin_types.h \
	$(srcdir)/include/driver_types.h \
	$(srcdir)/include/texture_types.h \
	$(srcdir)/include/surface_types.h \
	$(srcdir)/include/crt/host_defines.h \
	$(srcdir)/include/device_types.h \
	$(srcdir)/include/cuda_device_runtime_api.h \
	$(srcdir)/include/cuda_runtime_api.h \
	$(srcdir)/include/vector_types.h \
	$(srcdir)/include/cudaVDPAU.h
CUDA_HDR = \
	$(srcdir)/include/cuda.h \
	$(CUDA_EXTRA_HDR)

MODIFIED_CUDA_EXTRA_HDR = \
	modified_include/builtin_types.h \
	modified_include/driver_types.h \
	modified_include/texture_types.h \
	modified_include/surface_types.h \
	modified_include/crt/host_defines.h \
	modified_include/device_types.h \
	modified_include/cuda_device_runtime_api.h \
	modified_include/cuda_runtime_api.h \
	modified_include/vector_types.h \
	modified_include/cudaVDPAU.h
MODIFIED_CUDA_HDR = \
	modified_include/cuda.h \
	$(MODIFIED_CUDA_EXTRA_HDR)

modified_include/cuda.h: $(CUDA_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

$(MODIFIED_CUDA_EXTRA_HDR): modified_include/cuda.h
	@if test -f $@; then \
	  touch $@; \
	else \
	  $(RM) modified_include/cuda.h; \
	  $(MAKE) $(AM_MAKEFLAGS) modified_include/cuda.h; \
	fi

clean-local:
	$(RM) -r modified_include

EXTRA_DIST += $(srcdir)/include headers.patch

cuda_api.yaml: modified_include/cuda.h modified_include/cudaVDPAU.h $(CUDA_EXTRACT) $(srcdir)/extract_cuda.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_cuda.rb

cuda_exports_api.yaml: modified_include/cuda.h $(CUDA_EXPORTS_EXTRACT) $(srcdir)/extract_cuda_exports.rb cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_cuda_exports.rb

cudart_api.yaml: modified_include/cuda_runtime_api.h $(CUDA_EXTRACT) $(srcdir)/extract_cudart.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_cudart.rb

cuda_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_cuda_model.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

cuda_exports.h.include: $(srcdir)/gen_cude_exports_include.rb cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	cuda_babeltrace_model.yaml \
	cuda_exports.h.include

EXTRA_DIST += \
	extract_cuda.rb \
	extract_cuda_exports.rb \
	extract_cudart.rb

cuda_tracepoints.tp: $(srcdir)/gen_cuda_tracepoints.rb $(CUDA_MODEL) gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_tracepoints.rb > $@

cuda_exports_tracepoints.tp: $(srcdir)/gen_cuda_exports_tracepoints.rb $(CUDA_MODEL) gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_exports_tracepoints.rb > $@

cudart_tracepoints.tp: $(srcdir)/gen_cudart_tracepoints.rb $(CUDART_MODEL) gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cudart_tracepoints.rb > $@

cuda_args.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_args > $@

cuda_profiling.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_profiling > $@

cuda_properties.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_properties > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

EXTRA_DIST += \
	gen_cuda_tracepoints.rb \
	gen_cuda_exports_tracepoints.rb \
	gen_cudart_tracepoints.rb \
	gen_probe_base.rb \
	gen_cuda_custom_probes.rb

BUILT_SOURCES = $(CUDA_PROBES_INCL) $(CUDA_STATIC_PROBES_INCL) $(CUDART_PROBES_INCL)

tracer_cuda.c: $(srcdir)/gen_cuda.rb $(srcdir)/tracer_cuda_helpers.include.c $(CUDA_MODEL) $(CUDA_PROBES_INCL) $(CUDA_STATIC_PROBES_INCL) cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

tracer_cudart.c: $(srcdir)/gen_cudart.rb $(srcdir)/tracer_cudart_helpers.include.c $(CUDART_MODEL) $(CUDART_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_cuda.rb \
	tracer_cuda_helpers.include.c \
	gen_cudart.rb \
	tracer_cudart_helpers.include.c \
	cuda_export_tables.yaml

CLEANFILES += \
	tracer_cuda.c \
	tracer_cudart.c

bin_SCRIPTS = tracer_cuda.sh babeltrace_cuda

noinst_LTLIBRARIES = libcudatracepoints.la libcudarttracepoints.la

nodist_libcudatracepoints_la_SOURCES = \
	$(CUDA_PROBES_INCL) \
	$(CUDA_PROBES_SRC) \
	$(CUDA_STATIC_PROBES_INCL) \
	$(CUDA_STATIC_PROBES_SRC)

libcudatracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libcudatracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

nodist_libcudarttracepoints_la_SOURCES = \
	$(CUDART_PROBES_INCL) \
	$(CUDART_PROBES_SRC)

libcudarttracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libcudarttracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = libTracerCUDA.la libTracerCUDART.la libCUDAInterval.la

nodist_libTracerCUDA_la_SOURCES = \
	$(CUDA_PROBES_INCL) \
	tracer_cuda.c

libTracerCUDA_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libTracerCUDA_la_CFLAGS = -Wall -Wextra -Werror $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerCUDA_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerCUDA_la_LDFLAGS += -version-info 1:0:0
libTracerCUDA_la_LIBADD = libcudatracepoints.la

nodist_libTracerCUDART_la_SOURCES = \
	$(CUDART_PROBES_INCL) \
	tracer_cudart.c

libTracerCUDART_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./
libTracerCUDART_la_CFLAGS = -Wall -Wextra -Werror $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerCUDART_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerCUDART_la_LDFLAGS += -version-number 11:0:3
libTracerCUDART_la_LIBADD = libcudarttracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/cudart
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/cuda
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerCUDART.so.11.0.3 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.11.0
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.11.0 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerCUDA.so.1.0.0 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.11.0 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so
	$(RM) -f $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so
	-rmdir $(DESTDIR)$(pkglibdir)/cudart
	-rmdir $(DESTDIR)$(pkglibdir)/cuda

tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceCUDA.la
install-data-hook::
	$(RM) -r $(DESTDIR)$(tmplibdir)

CUDA_LIB_GEN = \
	$(srcdir)/gen_cuda_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += $(CUDA_LIB_GEN)

babeltrace_cuda_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb cuda_babeltrace_model.yaml cuda.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ cuda > $@

BUILT_SOURCES += babeltrace_cuda_callbacks.h

babeltrace_cuda_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ cuda > $@

EXTRA_DIST += cuda.h.include

CLEANFILES += \
	babeltrace_cuda_callbacks.h \
	babeltrace_cuda_dispatchers.c

nodist_libBabeltraceCUDA_la_SOURCES = \
	babeltrace_cuda_callbacks.h \
	babeltrace_cuda_dispatchers.c

libBabeltraceCUDA_la_SOURCES = \
	babeltrace_cuda.c \
	babeltrace_cuda.h
libBabeltraceCUDA_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libBabeltraceCUDA_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libBabeltraceCUDA_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

cuda_library.rb: $(srcdir)/gen_cuda_library.rb $(CUDA_LIB_GEN) $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_cuda_lib.rb: $(srcdir)/gen_babeltrace_cuda_lib.rb $(CUDA_LIB_GEN) $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CUDA_BINDINGS = \
	cuda_library.rb \
	babeltrace_cuda_lib.rb

TESTS_COMMON_TEMPLATE = \
	tests/interval_profiling_block.yaml \
	tests/profiling_block.yaml

$(TESTS_COMMON_TEMPLATE): tests/%.yaml: $(top_srcdir)/utils/tests_template/%.yaml.erb
	$(ERB) profiled_call="cuLaunchHostFunc" be="cuda" $< > $@

#https://www.gnu.org/software/make/manual/html_node/Static-Usage.html
INTERVAL_DUST_FILES = \
	tests/interval_profiling_block.dust

$(INTERVAL_DUST_FILES): tests/interval_%.dust: tests/interval.dust.erb
	$(ERB) namespace="$@" $(srcdir)/tests/interval.dust.erb > $@

nodist_libCUDAInterval_la_SOURCES = \
	$(INTERVAL_DUST_FILES)

TRACE_FILES = \
	tests/profiling_block.yaml \
	tests/interval_profiling_block.yaml

tests/cuda_dummy_trace.yaml: cuda_babeltrace_model.yaml
	mkdir -p tests
	$(RUBY) $(top_srcdir)/utils/gen_dummy_trace.rb cuda_babeltrace_model.yaml > $@

CLEANFILES += tests/cuda_dummy_trace.yaml

CLEANFILES += \
	$(nodist_libCUDAInterval_la_SOURCES) \
	$(INTERVAL_DUST_FILES) \
	$(TESTS_COMMON_TEMPLATE)

TESTS = \
	tests/cuda_dummy_trace.dust \
	$(INTERVAL_DUST_FILES)

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh

check_DATA = \
	test_wrapper.sh \
	tests/cuda_dummy_trace.yaml \
	$(TRACE_FILES)

data_DATA = $(CUDA_BINDINGS)

EXTRA_DIST += \
	gen_cuda_library.rb \
	gen_babeltrace_cuda_lib.rb \
	$(TRACE_FILES) \
	tests/interval.dust.erb 

CLEANFILES += $(CUDA_BINDINGS)

babeltrace_cudainterval_callbacks.cpp: $(srcdir)/cudainterval_callbacks.cpp.erb $(srcdir)/gen_cudainterval_callbacks.rb cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cudainterval_callbacks.rb  > $@

babeltrace_cudainterval_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb cuda_babeltrace_model.yaml cuda.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ cudainterval > $@

babeltrace_cudainterval_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ cudainterval > $@

babeltrace_cudainterval.h: $(top_srcdir)/xprof/interval.h.erb
	$(ERB) namespace="cudainterval" $(top_srcdir)/xprof/interval.h.erb > $@

babeltrace_cudainterval.c: $(top_srcdir)/xprof/interval.c.erb
	$(ERB) namespace="cudainterval"  $(top_srcdir)/xprof/interval.c.erb > $@

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

BUILT_SOURCES += \
	$(OPENCL_PROBES_INCL) \
	babeltrace_cudainterval.h \
	babeltrace_cudainterval_callbacks.h \
	babeltrace_cudainterval.c \
	xprof_utils.hpp 

nodist_libCUDAInterval_la_SOURCES += \
	babeltrace_cudainterval.c \
	babeltrace_cudainterval.h \
	babeltrace_cudainterval_callbacks.h \
	babeltrace_cudainterval_dispatchers.c \
	babeltrace_cudainterval_callbacks.cpp \
	tracer_cuda.h \
	xprof_utils.hpp \
	xprof_utils.cpp

EXTRA_DIST += \
	gen_cudainterval_callbacks.rb \
	cudainterval_callbacks.cpp.erb

libCUDAInterval_la_SOURCES = \
	cudainterval_callbacks.hpp

libCUDAInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libCUDAInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCUDAInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libCUDAInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

CLEANFILES += \
	$(nodist_libCUDAInterval_la_SOURCES)

cuprof.sh: $(top_builddir)/utils/xprof.sh.erb
	$(ERB) -T 1 languages=["cuda"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@

bin_SCRIPTS += cuprof.sh

CLEANFILES += cuprof.sh
