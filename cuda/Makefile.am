.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

CUDA_HDR = \
	$(srcdir)/include/cuda.h \
	$(srcdir)/include/builtin_types.h \
	$(srcdir)/include/driver_types.h \
	$(srcdir)/include/texture_types.h \
	$(srcdir)/include/surface_types.h \
	$(srcdir)/include/crt/host_defines.h \
	$(srcdir)/include/device_types.h \
	$(srcdir)/include/cuda_device_runtime_api.h \
	$(srcdir)/include/cuda_runtime_api.h \
	$(srcdir)/include/__cudart.h \
	$(srcdir)/include/cuda_vdpau_interop.h \
	$(srcdir)/include/cuda_profiler_api.h \
	$(srcdir)/include/vector_types.h \
	$(srcdir)/include/cudaVDPAU.h \
	$(srcdir)/include/cudaProfiler.h \
	$(srcdir)/include/cudaTypedefs.h \
	$(srcdir)/include/cudaVDPAUTypedefs.h \
	$(srcdir)/include/cudaProfilerTypedefs.h

MODIFIED_CUDA_HDR = \
	modified_include/cuda.h \
	modified_include/builtin_types.h \
	modified_include/driver_types.h \
	modified_include/texture_types.h \
	modified_include/surface_types.h \
	modified_include/crt/host_defines.h \
	modified_include/device_types.h \
	modified_include/cuda_device_runtime_api.h \
	modified_include/cuda_runtime_api.h \
	modified_include/__cudart.h \
	modified_include/cuda_vdpau_interop.h \
	modified_include/cuda_profiler_api.h \
	modified_include/vector_types.h \
	modified_include/cudaVDPAU.h \
	modified_include/cudaProfiler.h \
	modified_include/cudaTypedefs.h \
	modified_include/cudaVDPAUTypedefs.h \
	modified_include/cudaProfilerTypedefs.h

BTX_CUDA_GENERATED = \
	btx_filter_cuda/metababel/metababel.h \
	btx_filter_cuda/metababel/btx_component.h \
	btx_filter_cuda/metababel/btx_component.c \
	btx_filter_cuda/metababel/btx_upstream.h \
	btx_filter_cuda/metababel/btx_upstream.c \
	btx_filter_cuda/metababel/btx_downstream.h \
	btx_filter_cuda/metababel/btx_downstream.c \
	btx_filter_cuda/btx_main.c

EXTRA_DIST = \
	$(top_srcdir)/utils/babeltrace_to_metababel_model.rb \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	btx_cudamatching_model.yaml

CLEANFILES = \
	btx_cuda_model.yaml

btx_cuda_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_CUDA_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_cudamatching_model.yaml btx_cuda_model.yaml
	$(METABABEL) -u btx_cuda_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_cuda -p cudainterval -c interval --matching $(srcdir)/btx_cudamatching_model.yaml -i cuda.h.include

$(MODIFIED_CUDA_HDR) &: $(CUDA_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

clean-local:
	$(RM) -r modified_include
	$(RM) -r tests

EXTRA_DIST += $(srcdir)/include headers.patch

CUDA_EXTRACT = \
	$(srcdir)/extract_base.rb

cuda_api_versions.yaml: $(srcdir)/extract_cuda_versions.rb modified_include/cudaTypedefs.h modified_include/cudaVDPAUTypedefs.h modified_include/cudaProfilerTypedefs.h
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

cuda_api.yaml: $(srcdir)/extract_cuda.rb $(CUDA_EXTRACT) modified_include/cuda.h modified_include/cudaVDPAU.h
	SRC_DIR=$(srcdir) $(RUBY) $<

cuda_exports_api.yaml: $(srcdir)/extract_cuda_exports.rb $(CUDA_EXTRACT) modified_include/cuda.h cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $<

cudart_api.yaml: $(srcdir)/extract_cudart.rb $(CUDA_EXTRACT) modified_include/cuda_runtime_api.h
	SRC_DIR=$(srcdir) $(RUBY) $<

CUDA_EXTRACTED = \
	cuda_api.yaml \
	cuda_exports_api.yaml \
	cuda_api_versions.yaml

CUDART_EXTRACTED = \
	cudart_api.yaml

CLEANFILES += $(CUDA_EXTRACTED) $(CUDART_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	extract_cuda.rb \
	extract_cuda_exports.rb \
	extract_cuda_versions.rb \
	extract_cudart.rb

cuda_exports.h.include: $(srcdir)/gen_cude_exports_include.rb cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	cuda_exports.h.include

EXTRA_DIST += \
	gen_cude_exports_include.rb

ML_CUDA_FILES = \
	$(srcdir)/cuda_meta_parameters.yaml \
	$(srcdir)/cuda_exports_meta_parameters.yaml

ML_CUDART_FILES = \
	$(srcdir)/cudart_meta_parameters.yaml

EXTRA_DIST += $(ML_CUDA_FILES) $(ML_CUDART_FILES)

CUDA_MODEL = \
	$(srcdir)/cuda_model.rb \
	$(srcdir)/gen_cuda_library_base.rb \
	$(srcdir)/gen_probe_base.rb \
	$(ML_CUDA_FILES) \
	$(srcdir)/cuda_events.yaml \
	$(CUDA_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/yaml_ast_lttng.rb \
	$(srcdir)/../utils/meta_parameters.rb \
	$(srcdir)/../utils/LTTng.rb \
	$(srcdir)/../utils/command.rb \
	$(srcdir)/cuda.h.include \
	cuda_exports.h.include

CUDART_MODEL = \
	$(srcdir)/cudart_model.rb \
	$(ML_CUDART_FILES) \
	$(CUDART_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/yaml_ast_lttng.rb \
	$(srcdir)/../utils/meta_parameters.rb \
	$(srcdir)/../utils/LTTng.rb \
	$(srcdir)/cudart.h.include

cuda_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_cuda_model.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	cuda_babeltrace_model.yaml

EXTRA_DIST += \
	cuda_model.rb \
	gen_cuda_library_base.rb \
	gen_probe_base.rb \
	cuda_events.yaml \
	cudart_model.rb \
	gen_babeltrace_cuda_model.rb

CUDA_PROBES = \
	cuda_tracepoints \
	cuda_exports_tracepoints

CUDA_PROBES_TP = $(CUDA_PROBES:=.tp)

CUDA_PROBES_INCL = $(CUDA_PROBES:=.h)

CUDA_PROBES_SRC = $(CUDA_PROBES:=.c)

CUDA_STATIC_PROBES = \
	cuda_args \
	cuda_profiling \
	cuda_properties

CUDA_STATIC_PROBES_TP = $(CUDA_STATIC_PROBES:=.tp)

CUDA_STATIC_PROBES_INCL = $(CUDA_STATIC_PROBES:=.h)

CUDA_STATIC_PROBES_SRC = $(CUDA_STATIC_PROBES:=.c)

CUDART_PROBES = \
	cudart_tracepoints

CUDART_PROBES_TP = $(CUDART_PROBES:=.tp)

CUDART_PROBES_INCL = $(CUDART_PROBES:=.h)

CUDART_PROBES_SRC = $(CUDART_PROBES:=.c)

$(CUDA_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

$(CUDART_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(CUDART_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

$(CUDA_STATIC_PROBES_TP): %.tp: $(srcdir)/gen_cuda_custom_probes.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< lttng_ust_$* > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(CUDA_PROBES_TP) \
	$(CUDA_PROBES_INCL) \
	$(CUDA_PROBES_SRC) \
	$(CUDA_STATIC_PROBES_TP) \
	$(CUDA_STATIC_PROBES_INCL) \
	$(CUDA_STATIC_PROBES_SRC) \
	$(CUDART_PROBES_TP) \
	$(CUDART_PROBES_INCL) \
	$(CUDART_PROBES_SRC)

EXTRA_DIST += \
	gen_cuda_tracepoints.rb \
	gen_cuda_exports_tracepoints.rb \
	gen_cudart_tracepoints.rb \
	gen_cuda_custom_probes.rb

BUILT_SOURCES = \
	$(CUDA_PROBES_INCL) \
	$(CUDA_STATIC_PROBES_INCL) \
	$(CUDART_PROBES_INCL) \
	tracer_cudart.map

tracer_cuda.c: $(srcdir)/gen_cuda.rb $(srcdir)/tracer_cuda_helpers.include.c $(CUDA_MODEL) $(CUDA_PROBES_INCL) $(CUDA_STATIC_PROBES_INCL) cuda_export_tables.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

tracer_cudart.c: $(srcdir)/gen_cudart.rb $(srcdir)/tracer_cudart_helpers.include.c $(CUDART_MODEL) $(CUDART_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

tracer_cudart.map: $(srcdir)/gen_cudart_linker_script.rb $(CUDART_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< libcudart.so.12 > $@

EXTRA_DIST += \
	gen_cuda.rb \
	tracer_cuda_helpers.include.c \
	cuda_export_tables.yaml \
	gen_cudart.rb \
	gen_cudart_linker_script.rb \
	tracer_cudart_helpers.include.c

CLEANFILES += \
	tracer_cuda.c \
	tracer_cudart.c \
	tracer_cudart.map

bin_SCRIPTS = \
	tracer_cuda.sh

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

libTracerCUDA_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I$(srcdir)/include -I../utils -I./
libTracerCUDA_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerCUDA_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS) ../sampling/libThapiSampling.la
libTracerCUDA_la_LDFLAGS += -version-info 1:0:0
libTracerCUDA_la_LIBADD = libcudatracepoints.la

nodist_libTracerCUDART_la_SOURCES = \
	$(CUDART_PROBES_INCL) \
	tracer_cudart.c

libTracerCUDART_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I$(srcdir)/include -I../utils -I./
libTracerCUDART_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerCUDART_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS) ../sampling/libThapiSampling.la
libTracerCUDART_la_LDFLAGS += -version-number 12:1:55 -Wl,--version-script,tracer_cudart.map
libTracerCUDART_la_LIBADD = libcudarttracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/cudart
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/cuda
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerCUDART.so.12.1.55 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.12
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.12 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerCUDA.so.1.0.0 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libCUDAInterval.so $(DESTDIR)$(pkglibdir)/bt2/libCUDAInterval.so


uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/cudart/libcudart.so.12 $(DESTDIR)$(pkglibdir)/cudart/libcudart.so
	$(RM) -f $(DESTDIR)$(pkglibdir)/cuda/libcuda.so.1 $(DESTDIR)$(pkglibdir)/cuda/libcuda.so
	-rmdir $(DESTDIR)$(pkglibdir)/cudart
	-rmdir $(DESTDIR)$(pkglibdir)/cuda
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libCUDAInterval.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

tmplibdir = $(libdir)/tmp
tmplib_LTLIBRARIES = libBabeltraceCUDA.la
install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

babeltrace_cuda_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb cuda_babeltrace_model.yaml cuda.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ cuda > $@

BUILT_SOURCES += babeltrace_cuda_callbacks.h

babeltrace_cuda_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ cuda > $@

EXTRA_DIST += cuda.h.include cudart.h.include

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
libBabeltraceCUDA_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libBabeltraceCUDA_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

cuda_library.rb: $(srcdir)/gen_cuda_library.rb $(CUDA_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_cuda_lib.rb: $(srcdir)/gen_babeltrace_cuda_lib.rb $(srcdir)/../utils/gen_library_base.rb $(CUDA_MODEL) cuda_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CUDA_BINDINGS = \
	cuda_library.rb \
	babeltrace_cuda_lib.rb

EXTRA_DIST += \
	gen_cuda_library.rb \
	gen_babeltrace_cuda_lib.rb

CLEANFILES += $(CUDA_BINDINGS)

data_DATA = $(CUDA_BINDINGS)

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
	$(BTX_CUDA_GENERATED) \
	xprof_utils.cpp \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_CUDA_GENERATED) \
	xprof_utils.hpp

nodist_libCUDAInterval_la_SOURCES = \
	$(BTX_CUDA_GENERATED) \
	xprof_utils.hpp \
	xprof_utils.cpp

libCUDAInterval_la_SOURCES = \
	btx_cudainterval_callbacks.cpp \
	btx_cudainterval_callbacks.hpp \
	context_manager.cpp \
	context_manager.hpp

libCUDAInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_cuda
libCUDAInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCUDAInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libCUDAInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

tests/.tag:
	mkdir -p tests
	touch tests/.tag

CLEANFILES += tests/.tag

tests/cuda_dummy_trace.yaml: $(top_srcdir)/utils/gen_dummy_trace.rb cuda_babeltrace_model.yaml tests/.tag
	$(RUBY) $< cuda_babeltrace_model.yaml > $@

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
	tests/interval_profiling_normal.thapi_text_pretty \
	tests/interval_profiling_cuda_driver_api.thapi_text_pretty \
	tests/interval_profiling_cuda_multictx.thapi_text_pretty \
	tests/interval_profiling_cuda_multistream.thapi_text_pretty \
	tests/interval_profiling_cuda_memcpy.thapi_text_pretty \
	tests/interval_profiling_multithread.thapi_text_pretty \
	tests/interval_profiling_kernel_name.thapi_text_pretty

BTX_CUDA_GENERATED_SOURCE_TEST = \
	btx_source_cuda_test/metababel/metababel.h \
	btx_source_cuda_test/metababel/btx_component.h \
	btx_source_cuda_test/metababel/btx_component.c \
	btx_source_cuda_test/metababel/btx_downstream.h \
	btx_source_cuda_test/metababel/btx_downstream.c \
	btx_source_cuda_test/btx_main.c

$(BTX_CUDA_GENERATED_SOURCE_TEST) &: btx_cuda_model.yaml
	$(METABABEL) -d btx_cuda_model.yaml -t SOURCE  -o btx_source_cuda_test -p cudatest -c test  -i cuda.h.include

noinst_LTLIBRARIES += libtestcudasource.la
libtestcudasource_la_SOURCES = $(BTX_CUDA_GENERATED_SOURCE_TEST)
libtestcudasource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_cuda_test/
libtestcudasource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

TRACE_OUT = $(TRACE_COMMON:.thapi_text_pretty=.bt_text_pretty)

TESTS = \
	tests/cuda_dummy_trace.dust \
	$(TRACE_COMMON)

TEST_EXTENSIONS = .thapi_text_pretty .dust
THAPI_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_TEXT_PRETTY_LOG_FLAGS = cuda cuda ./btx_source_cuda_test/ .libs/libtestcudasource.a
DUST_LOG_COMPILER = ./test_wrapper.sh

check_DATA = \
	test_wrapper.sh \
	tests/cuda_dummy_trace.yaml \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

EXTRA_DIST += \
	tests/cuda_dummy_trace.dust \
	tests/interval.dust.erb \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

CLEANFILES += \
	tests/cuda_dummy_trace.yaml \
	$(TRACE_COMMON) \
	$(BTX_CUDA_GENERATED_SOURCE_TEST)
