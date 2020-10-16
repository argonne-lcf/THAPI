.DELETE_ON_ERROR:

LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./

CUDA_PROBES_TP = \
	cuda_tracepoints.tp

CUDA_PROBES_INCL = \
	cuda_tracepoints.h

CUDA_PROBES_SRC = \
	cuda_tracepoints.c

CUDA_STATIC_PROBES_TP = \
	cuda_args.tp \
	cuda_profiling.tp \
	cuda_exports.tp

CUDA_STATIC_PROBES_INCL = \
	cuda_args.h \
	cuda_profiling.h \
	cuda_exports.h

CUDA_STATIC_PROBES_SRC = \
	cuda_args.c \
	cuda_profiling.c \
	cuda_exports.c

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
	$(srcdir)/cuda_meta_parameters.yaml

ML_CUDART_FILES = \
	$(srcdir)/cudart_meta_parameters.yaml

EXTRA_DIST = $(ML_CUDA_FILES) $(ML_CUDART_FILES)

CUDA_EXTRACT = \
	$(srcdir)/extract_base.rb

EXTRA_DIST += extract_base.rb

CUDA_EXTRACTED = \
	cuda_api.yaml

CUDART_EXTRACTED = \
	cudart_api.yaml

CLEANFILES += $(CUDA_EXTRACTED) $(CUDART_EXTRACTED)

CUDA_MODEL = \
	$(srcdir)/cuda_model.rb \
	$(ML_CUDA_FILES) \
	$(srcdir)/cuda_events.yaml \
	$(CUDA_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/LTTng.rb

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
	$(srcdir)/include/vector_types.h
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
	modified_include/vector_types.h
MODIFIED_CUDA_HDR = \
	modified_include/cuda.h \
	$(MODIFIED_CUDA_EXTRA_HDR)

modified_include/cuda.h: $(CUDA_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
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

cuda_api.yaml: modified_include/cuda.h $(CUDA_EXTRACT) $(srcdir)/extract_cuda.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_cuda.rb

cudart_api.yaml: modified_include/cuda_runtime_api.h $(CUDA_EXTRACT) $(srcdir)/extract_cudart.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/extract_cudart.rb

EXTRA_DIST += \
	extract_cuda.rb \
	extract_cudart.rb

cuda_tracepoints.tp: $(srcdir)/gen_cuda_tracepoints.rb $(CUDA_MODEL) gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_tracepoints.rb > $@

cudart_tracepoints.tp: $(srcdir)/gen_cudart_tracepoints.rb $(CUDART_MODEL) gen_probe_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cudart_tracepoints.rb > $@

cuda_args.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_args > $@

cuda_profiling.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_profiling > $@

cuda_exports.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $(srcdir)/gen_cuda_custom_probes.rb lttng_ust_cuda_exports > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

EXTRA_DIST += \
	gen_cuda_tracepoints.rb \
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

lib_LTLIBRARIES = libTracerCUDA.la libTracerCUDART.la

nodist_libTracerCUDA_la_SOURCES = \
	$(CUDA_PROBES_INCL) \
	tracer_cuda.c

libTracerCUDA_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./
libTracerCUDA_la_CFLAGS = -Wall -Wextra -Werror
libTracerCUDA_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerCUDA_la_LDFLAGS += -version-info 1:0:0
libTracerCUDA_la_LIBADD = libcudatracepoints.la

nodist_libTracerCUDART_la_SOURCES = \
	$(CUDART_PROBES_INCL) \
	tracer_cudart.c

libTracerCUDART_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I../utils -I./
libTracerCUDART_la_CFLAGS = -Wall -Wextra -Werror
libTracerCUDART_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS)
libTracerCUDART_la_LDFLAGS += -version-number 10:1:0
libTracerCUDART_la_LIBADD = libcudarttracepoints.la

install-exec-hook::
	mkdir -p $(pkglibdir)
	mkdir -p $(pkglibdir)/cuda
	mkdir -p $(pkglibdir)/cudart
	$(RM) $(pkglibdir)/cudart/libcudart.so.10.1 $(pkglibdir)/cudart/libcudart.so
	$(RM) $(pkglibdir)/cuda/libcuda.so.1 $(pkglibdir)/cuda/libcuda.so
	ln -s $(libdir)/libTracerCUDART.so.10.1.0 $(pkglibdir)/cudart/libcudart.so.10.1
	ln -s $(pkglibdir)/cudart/libcudart.so.10.1 $(pkglibdir)/cudart/libcudart.so
	ln -s $(libdir)/libTracerCUDA.so.1.0.0 $(pkglibdir)/cuda/libcuda.so.1
	ln -s $(pkglibdir)/cuda/libcuda.so.1 $(pkglibdir)/cuda/libcuda.so

CUDA_LIB_GEN = \
	$(srcdir)/gen_cuda_library_base.rb \
	$(srcdir)/gen_probe_base.rb

EXTRA_DIST += $(CUDA_LIB_GEN)

cuda_library.rb: $(srcdir)/gen_cuda_library.rb $(CUDA_LIB_GEN) $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_cuda_lib.rb: $(srcdir)/gen_babeltrace_cuda_lib.rb $(CUDA_LIB_GEN) $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CUDA_BINDINGS = \
	cuda_library.rb \
	babeltrace_cuda_lib.rb

data_DATA = $(CUDA_BINDINGS)

EXTRA_DIST += \
	gen_cuda_library.rb \
	gen_babeltrace_cuda_lib.rb

CLEANFILES += $(CUDA_BINDINGS)
