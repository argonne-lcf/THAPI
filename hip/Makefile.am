.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

HIP_HDR = \
	$(srcdir)/include/hip/hip_runtime_api.h \
	$(srcdir)/include/hip/hip_cooperative_groups.h \
	$(srcdir)/include/hip/device_functions.h \
	$(srcdir)/include/hip/driver_types.h \
	$(srcdir)/include/hip/hip_math_constants.h \
	$(srcdir)/include/hip/texture_types.h \
	$(srcdir)/include/hip/math_functions.h \
	$(srcdir)/include/hip/hip_vector_types.h \
	$(srcdir)/include/hip/hip_fp16.h \
	$(srcdir)/include/hip/hip_complex.h \
	$(srcdir)/include/hip/hip_hcc.h \
	$(srcdir)/include/hip/surface_types.h \
	$(srcdir)/include/hip/library_types.h \
	$(srcdir)/include/hip/hip_bfloat16.h \
	$(srcdir)/include/hip/amd_detail/amd_warp_functions.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_vector_types.h \
	$(srcdir)/include/hip/amd_detail/amd_math_functions.h \
	$(srcdir)/include/hip/amd_detail/amd_device_functions.h \
	$(srcdir)/include/hip/amd_detail/hip_runtime_prof.h \
	$(srcdir)/include/hip/amd_detail/concepts.hpp \
	$(srcdir)/include/hip/amd_detail/amd_hip_math_constants.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_runtime_pt_api.h \
	$(srcdir)/include/hip/amd_detail/grid_launch.hpp \
	$(srcdir)/include/hip/amd_detail/texture_fetch_functions.h \
	$(srcdir)/include/hip/amd_detail/llvm_intrinsics.h \
	$(srcdir)/include/hip/amd_detail/amd_surface_functions.h \
	$(srcdir)/include/hip/amd_detail/grid_launch_GGL.hpp \
	$(srcdir)/include/hip/amd_detail/hip_ldg.h \
	$(srcdir)/include/hip/amd_detail/host_defines.h \
	$(srcdir)/include/hip/amd_detail/hip_cooperative_groups_helper.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_cooperative_groups.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_complex.h \
	$(srcdir)/include/hip/amd_detail/texture_indirect_functions.h \
	$(srcdir)/include/hip/amd_detail/math_fwd.h \
	$(srcdir)/include/hip/amd_detail/device_library_decls.h \
	$(srcdir)/include/hip/amd_detail/grid_launch.h \
	$(srcdir)/include/hip/amd_detail/functional_grid_launch.hpp \
	$(srcdir)/include/hip/amd_detail/macro_based_grid_launch.hpp \
	$(srcdir)/include/hip/amd_detail/hip_fp16_math_fwd.h \
	$(srcdir)/include/hip/amd_detail/helpers.hpp \
	$(srcdir)/include/hip/amd_detail/amd_hip_fp16.h \
	$(srcdir)/include/hip/amd_detail/hsa_helpers.hpp \
	$(srcdir)/include/hip/amd_detail/hip_fp16_gcc.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_atomic.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_unsafe_atomics.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_runtime.h \
	$(srcdir)/include/hip/amd_detail/hip_prof_str.h \
	$(srcdir)/include/hip/amd_detail/amd_hip_common.h \
	$(srcdir)/include/hip/amd_detail/program_state.hpp \
	$(srcdir)/include/hip/amd_detail/amd_channel_descriptor.h \
	$(srcdir)/include/hip/amd_detail/ockl_image.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hiprtc.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_math_constants.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_atomics.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_runtime_api.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_cooperative_groups.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_channel_descriptor.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_unsafe_atomics.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_complex.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_runtime.h \
	$(srcdir)/include/hip/nvidia_detail/nvidia_hip_texture_types.h \
	$(srcdir)/include/hip/hip_profile.h \
	$(srcdir)/include/hip/hip_texture_types.h \
	$(srcdir)/include/hip/hip_common.h \
	$(srcdir)/include/hip/hip_runtime.h \
	$(srcdir)/include/hip/hip_ext.h \
	$(srcdir)/include/hip/hiprtc.h \
	$(srcdir)/include/hip/channel_descriptor.h \
	$(srcdir)/include/hip/hip_version.h

MODIFIED_HIP_HDR = \
	modified_include/hip/hip_runtime_api.h \
	modified_include/hip/hip_cooperative_groups.h \
	modified_include/hip/device_functions.h \
	modified_include/hip/driver_types.h \
	modified_include/hip/hip_math_constants.h \
	modified_include/hip/texture_types.h \
	modified_include/hip/math_functions.h \
	modified_include/hip/hip_vector_types.h \
	modified_include/hip/hip_fp16.h \
	modified_include/hip/hip_complex.h \
	modified_include/hip/hip_hcc.h \
	modified_include/hip/surface_types.h \
	modified_include/hip/library_types.h \
	modified_include/hip/hip_bfloat16.h \
	modified_include/hip/amd_detail/amd_warp_functions.h \
	modified_include/hip/amd_detail/amd_hip_vector_types.h \
	modified_include/hip/amd_detail/amd_math_functions.h \
	modified_include/hip/amd_detail/amd_device_functions.h \
	modified_include/hip/amd_detail/hip_runtime_prof.h \
	modified_include/hip/amd_detail/concepts.hpp \
	modified_include/hip/amd_detail/amd_hip_math_constants.h \
	modified_include/hip/amd_detail/amd_hip_runtime_pt_api.h \
	modified_include/hip/amd_detail/grid_launch.hpp \
	modified_include/hip/amd_detail/texture_fetch_functions.h \
	modified_include/hip/amd_detail/llvm_intrinsics.h \
	modified_include/hip/amd_detail/amd_surface_functions.h \
	modified_include/hip/amd_detail/grid_launch_GGL.hpp \
	modified_include/hip/amd_detail/hip_ldg.h \
	modified_include/hip/amd_detail/host_defines.h \
	modified_include/hip/amd_detail/hip_cooperative_groups_helper.h \
	modified_include/hip/amd_detail/amd_hip_cooperative_groups.h \
	modified_include/hip/amd_detail/amd_hip_complex.h \
	modified_include/hip/amd_detail/texture_indirect_functions.h \
	modified_include/hip/amd_detail/math_fwd.h \
	modified_include/hip/amd_detail/device_library_decls.h \
	modified_include/hip/amd_detail/grid_launch.h \
	modified_include/hip/amd_detail/functional_grid_launch.hpp \
	modified_include/hip/amd_detail/macro_based_grid_launch.hpp \
	modified_include/hip/amd_detail/hip_fp16_math_fwd.h \
	modified_include/hip/amd_detail/helpers.hpp \
	modified_include/hip/amd_detail/amd_hip_fp16.h \
	modified_include/hip/amd_detail/hsa_helpers.hpp \
	modified_include/hip/amd_detail/hip_fp16_gcc.h \
	modified_include/hip/amd_detail/amd_hip_atomic.h \
	modified_include/hip/amd_detail/amd_hip_unsafe_atomics.h \
	modified_include/hip/amd_detail/amd_hip_runtime.h \
	modified_include/hip/amd_detail/hip_prof_str.h \
	modified_include/hip/amd_detail/amd_hip_common.h \
	modified_include/hip/amd_detail/program_state.hpp \
	modified_include/hip/amd_detail/amd_channel_descriptor.h \
	modified_include/hip/amd_detail/ockl_image.h \
	modified_include/hip/nvidia_detail/nvidia_hiprtc.h \
	modified_include/hip/nvidia_detail/nvidia_hip_math_constants.h \
	modified_include/hip/nvidia_detail/nvidia_hip_atomics.h \
	modified_include/hip/nvidia_detail/nvidia_hip_runtime_api.h \
	modified_include/hip/nvidia_detail/nvidia_hip_cooperative_groups.h \
	modified_include/hip/nvidia_detail/nvidia_channel_descriptor.h \
	modified_include/hip/nvidia_detail/nvidia_hip_unsafe_atomics.h \
	modified_include/hip/nvidia_detail/nvidia_hip_complex.h \
	modified_include/hip/nvidia_detail/nvidia_hip_runtime.h \
	modified_include/hip/nvidia_detail/nvidia_hip_texture_types.h \
	modified_include/hip/hip_profile.h \
	modified_include/hip/hip_texture_types.h \
	modified_include/hip/hip_common.h \
	modified_include/hip/hip_runtime.h \
	modified_include/hip/hip_ext.h \
	modified_include/hip/hiprtc.h \
	modified_include/hip/channel_descriptor.h \
	modified_include/hip/hip_version.h

BTX_HIP_GENERATED = \
	btx_filter_hip/metababel/metababel.h \
	btx_filter_hip/metababel/btx_component.h \
	btx_filter_hip/metababel/btx_component.c \
	btx_filter_hip/metababel/btx_upstream.h \
	btx_filter_hip/metababel/btx_upstream.c \
	btx_filter_hip/metababel/btx_downstream.h \
	btx_filter_hip/metababel/btx_downstream.c \
	btx_filter_hip/btx_main.c

EXTRA_DIST = \
	$(top_srcdir)/utils/babeltrace_to_metababel_model.rb \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	btx_hipmatching_model.yaml

CLEANFILES = \
	btx_hip_model.yaml

btx_hip_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb hip_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_HIP_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_hipmatching_model.yaml btx_hip_model.yaml
	$(METABABEL) -u btx_hip_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_hip -p hipinterval -c interval --matching $(srcdir)/btx_hipmatching_model.yaml -i hip.h.include

$(MODIFIED_HIP_HDR) &: $(HIP_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

clean-local:
	$(RM) -r modified_include
	$(RM) -r tests

EXTRA_DIST += \
	$(srcdir)/include \
	headers.patch

HIP_EXTRACT = \
	$(srcdir)/extract_base.rb

hip_api.yaml: $(srcdir)/extract_hip.rb $(HIP_EXTRACT) $(MODIFIED_HIP_HDR) $(srcdir)/hip_cpp_symbols.h $(srcdir)/hip_missing_apis.h
	SRC_DIR=$(srcdir) $(RUBY) $<

HIP_EXTRACTED = \
	hip_api.yaml

CLEANFILES += \
	$(HIP_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	extract_hip.rb \
	hip_cpp_symbols.h \
	hip_missing_apis.h

ML_HIP_FILES = \
	$(srcdir)/hip_meta_parameters.yaml

EXTRA_DIST += \
	$(ML_HIP_FILES)

HIP_MODEL = \
	$(srcdir)/hip_model.rb \
	$(srcdir)/gen_hip_library_base.rb \
	$(srcdir)/gen_probe_base.rb \
	$(ML_HIP_FILES) \
	$(srcdir)/hip_events.yaml \
	$(HIP_EXTRACTED) \
	$(srcdir)/../utils/yaml_ast.rb \
	$(srcdir)/../utils/yaml_ast_lttng.rb \
	$(srcdir)/../utils/meta_parameters.rb \
	$(srcdir)/../utils/LTTng.rb \
	hip.h.include

hip_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_hip_model.rb $(HIP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	hip_babeltrace_model.yaml

EXTRA_DIST += \
	hip_model.rb \
	gen_hip_library_base.rb \
	gen_probe_base.rb \
	hip_events.yaml \
	gen_babeltrace_hip_model.rb

HIP_PROBES = \
	hip_tracepoints

HIP_PROBES_TP = $(HIP_PROBES:=.tp)

HIP_PROBES_INCL = $(HIP_PROBES:=.h)

HIP_PROBES_SRC = $(HIP_PROBES:=.c)

$(HIP_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(HIP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(HIP_PROBES_TP) \
	$(HIP_PROBES_INCL) \
	$(HIP_PROBES_SRC)

EXTRA_DIST += \
	gen_hip_tracepoints.rb

BUILT_SOURCES = \
	$(HIP_PROBES_INCL)

tracer_hip.c: $(srcdir)/gen_hip.rb $(srcdir)/tracer_hip_helpers.include.c $(HIP_MODEL) $(HIP_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_hip.rb \
	tracer_hip_helpers.include.c

CLEANFILES += \
	tracer_hip.c

bin_SCRIPTS = \
	tracer_hip.sh

noinst_LTLIBRARIES = \
	libhiptracepoints.la

nodist_libhiptracepoints_la_SOURCES = \
	$(HIP_PROBES_INCL) \
	$(HIP_PROBES_SRC)

libhiptracepoints_la_CFLAGS = $(LTTNG_FLAGS) $(LTTNG_UST_CFLAGS)
libhiptracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

lib_LTLIBRARIES = \
	libTracerHIP.la \
	libHIPInterval.la

nodist_libTracerHIP_la_SOURCES = \
	$(HIP_PROBES_INCL) \
	tracer_hip.c

libTracerHIP_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I$(srcdir)/include -I./utils -I./
libTracerHIP_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerHIP_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS) ../sampling/libThapiSampling.la
libTracerHIP_la_LDFLAGS += -Wl,--version-script,$(srcdir)/hip.map -version-number 5:4:50400
libTracerHIP_la_DEPENDS = $(srcdir)/hip.map
libTracerHIP_la_LIBADD = libhiptracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/hip
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerHIP.so.5.4.50400 $(DESTDIR)$(pkglibdir)/hip/libamdhip64.so.5
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/hip/libamdhip64.so.5 $(DESTDIR)$(pkglibdir)/hip/libamdhip64.so
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libHIPInterval.so $(DESTDIR)$(pkglibdir)/bt2/libHIPInterval.so


uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/hip/libamdhip64.so.5 $(DESTDIR)$(pkglibdir)/hip/libamdhip64.so
	-rmdir $(DESTDIR)$(pkglibdir)/hip
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libHIPInterval.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

tmplibdir = $(libdir)/tmp

tmplib_LTLIBRARIES = libBabeltraceHIP.la

install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

babeltrace_hip_callbacks.h: $(top_srcdir)/utils/gen_babeltrace_callbacks.rb hip_babeltrace_model.yaml hip.h.include
	SRC_DIR=$(srcdir) $(RUBY) $^ hip > $@

babeltrace_hip_dispatchers.c: $(top_srcdir)/utils/gen_babeltrace_dispatchers.rb hip_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ hip > $@

EXTRA_DIST += \
	hip.h.include \
	hip.map

BUILT_SOURCES += \
	babeltrace_hip_callbacks.h

CLEANFILES += \
	babeltrace_hip_callbacks.h \
	babeltrace_hip_dispatchers.c

nodist_libBabeltraceHIP_la_SOURCES = \
	babeltrace_hip_callbacks.h \
	babeltrace_hip_dispatchers.c

libBabeltraceHIP_la_SOURCES = \
	babeltrace_hip.c \
	babeltrace_hip.h

libBabeltraceHIP_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libBabeltraceHIP_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libBabeltraceHIP_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

hip_library.rb: $(srcdir)/gen_hip_library.rb $(HIP_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_hip_lib.rb: $(srcdir)/gen_babeltrace_hip_lib.rb $(srcdir)/../utils/gen_library_base.rb $(HIP_MODEL) hip_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

HIP_BINDINGS = \
	hip_library.rb \
	babeltrace_hip_lib.rb

EXTRA_DIST += \
	gen_hip_library.rb \
	gen_babeltrace_hip_lib.rb

CLEANFILES += \
	$(HIP_BINDINGS)

data_DATA = \
	$(HIP_BINDINGS)

xprof_utils.cpp: $(top_srcdir)/utils/xprof_utils.cpp
	cp $< $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

CLEANFILES += \
	$(BTX_HIP_GENERATED) \
	xprof_utils.cpp \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_HIP_GENERATED)

nodist_libHIPInterval_la_SOURCES = \
	$(BTX_HIP_GENERATED) \
	xprof_utils.hpp

libHIPInterval_la_SOURCES = \
	btx_hipinterval_callbacks.cpp

libHIPInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_hip
libHIPInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libHIPInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libHIPInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

tests/.tag:
	mkdir -p tests
	touch tests/.tag

CLEANFILES += \
	tests/.tag

tests/hip_dummy_trace.yaml: $(top_srcdir)/utils/gen_dummy_trace.rb hip_babeltrace_model.yaml tests/.tag
	$(RUBY) $< hip_babeltrace_model.yaml > $@

TRACE_COMMON = \
	tests/interval_profiling_simple.thapi_text_pretty

BTX_HIP_GENERATED_SOURCE_TEST = \
	btx_source_hip_test/metababel/metababel.h \
	btx_source_hip_test/metababel/btx_component.h \
	btx_source_hip_test/metababel/btx_component.c \
	btx_source_hip_test/metababel/btx_downstream.h \
	btx_source_hip_test/metababel/btx_downstream.c \
	btx_source_hip_test/btx_main.c

$(BTX_HIP_GENERATED_SOURCE_TEST) &: btx_hip_model.yaml
	$(METABABEL) -d btx_hip_model.yaml -t SOURCE  -o btx_source_hip_test -p hiptest -c test  -i hip.h.include

noinst_LTLIBRARIES += libtesthipsource.la
libtesthipsource_la_SOURCES = $(BTX_HIP_GENERATED_SOURCE_TEST)
libtesthipsource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_hip_test/
libtesthipsource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

TRACE_OUT = $(TRACE_COMMON:.thapi_text_pretty=.bt_text_pretty)

TRACE_BUILD_DIRS = $(TRACE_COMMON:.thapi_text_pretty=)

TESTS = \
	tests/hip_dummy_trace.dust \
	$(TRACE_COMMON)

TEST_EXTENSIONS = .dust .thapi_text_pretty
DUST_LOG_COMPILER = ./test_wrapper.sh
THAPI_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_TEXT_PRETTY_LOG_FLAGS = hip hip ./btx_source_hip_test/ .libs/libtesthipsource.a

check_DATA = \
	test_wrapper.sh \
	tests/hip_dummy_trace.yaml

EXTRA_DIST += \
	tests/hip_dummy_trace.dust \
	$(TRACE_COMMON) \
	$(TRACE_OUT)

CLEANFILES += \
	tests/hip_dummy_trace.yaml \
	$(BTX_HIP_GENERATED_SOURCE_TEST)
