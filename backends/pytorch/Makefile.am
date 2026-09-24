if STRICT
  WERROR = -Werror
else
  WERROR =
endif

PYTORCH_STATIC_PROBES = pytorch_tracepoints

PYTORCH_STATIC_PROBES_TP = $(PYTORCH_STATIC_PROBES:=.tp)

PYTORCH_STATIC_PROBES_INCL = $(PYTORCH_STATIC_PROBES:=.h)

PYTORCH_STATIC_PROBES_SRC = $(PYTORCH_STATIC_PROBES:=.c)

$(PYTORCH_STATIC_PROBES_TP): %.tp: $(top_srcdir)/utils/gen_custom_probes.rb $(srcdir)/pytorch_events.yaml $(top_srcdir)/utils/gen_probe_base.rb
	$(RUBY) $< $(srcdir)/pytorch_events.yaml lttng_ust_pytorch > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

noinst_LTLIBRARIES = libpytorchtracepoints.la

nodist_libpytorchtracepoints_la_SOURCES = \
	$(PYTORCH_STATIC_PROBES_INCL) \
	$(PYTORCH_STATIC_PROBES_SRC)

libpytorchtracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./
libpytorchtracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libpytorchtracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

EXTRA_DIST = \
	pytorch_events.yaml \
	$(top_srcdir)/utils/gen_custom_probes.rb \
	include

CLEANFILES = \
	$(PYTORCH_STATIC_PROBES_TP) \
	$(PYTORCH_STATIC_PROBES_INCL) \
	$(PYTORCH_STATIC_PROBES_SRC)

BUILT_SOURCES = \
	$(PYTORCH_STATIC_PROBES_INCL)

# Only -ltorch_cpu is needed: at::addGlobalCallback, at::RecordFunction::name
# and at::RecordFuntion::overload_name symbols must be resolved to register 
# entry/exit callbacks at load time.
DUMMY_TORCH_LIBS = \
	dummy_libs/libtorch_cpu.so

dummy_libs/libtorch_cpu.so:
	$(MKDIR_P) dummy_libs
	$(CXX) -shared -o $@ -x c++ /dev/null

install-exec-hook:
	-$(RM) $(DUMMY_TORCH_LIBS)
	-rmdir dummy_libs

clean-local:
	-$(RM) $(DUMMY_TORCH_LIBS)
	-rmdir dummy_libs

bin_SCRIPTS = \
	tracer_pytorch.sh

lib_LTLIBRARIES = libTracerPytorch.la

nodist_libTracerPytorch_la_SOURCES = \
	$(PYTORCH_STATIC_PROBES_INCL)

libTracerPytorch_la_SOURCES = \
	tracer_pytorch.cpp

libTracerPytorch_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libTracerPytorch_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-private-field $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerPytorch_la_LDFLAGS = $(LTTNG_UST_LIBS) -Ldummy_libs -ltorch_cpu -avoid-version -module
libTracerPytorch_la_LIBADD = libpytorchtracepoints.la
EXTRA_libTracerPytorch_la_DEPENDENCIES = $(DUMMY_TORCH_LIBS)

BTX_PYTORCH_GENERATED = \
	btx_filter_pytorch/metababel/metababel.h \
	btx_filter_pytorch/metababel/btx_component.h \
	btx_filter_pytorch/metababel/btx_component.c \
	btx_filter_pytorch/metababel/btx_upstream.h \
	btx_filter_pytorch/metababel/btx_upstream.c \
	btx_filter_pytorch/metababel/btx_downstream.h \
	btx_filter_pytorch/metababel/btx_downstream.c \
	btx_filter_pytorch/btx_main.c

EXTRA_DIST += \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	$(srcdir)/btx_pytorch_model.yaml

$(BTX_PYTORCH_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml $(srcdir)/btx_pytorch_model.yaml
	$(METABABEL) -u $(srcdir)/btx_pytorch_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_pytorch -p pytorchinterval -c interval

CLEANFILES += \
	$(BTX_PYTORCH_GENERATED)

BUILT_SOURCES += \
	$(BTX_PYTORCH_GENERATED)

nodist_libPyTorchInterval_la_SOURCES = \
	$(BTX_PYTORCH_GENERATED)

libPyTorchInterval_la_SOURCES = \
	btx_pytorchinterval_callbacks.cpp

libPyTorchInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I./ -I./btx_filter_pytorch
libPyTorchInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libPyTorchInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libPyTorchInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

bt2dir = $(pkglibdir)/bt2
bt2_LTLIBRARIES = libPyTorchInterval.la
