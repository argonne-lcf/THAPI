if STRICT
  WERROR = -Werror
else
  WERROR =
endif

PYTORCH_STATIC_PROBES = pytorch

PYTORCH_STATIC_PROBES_TP = $(PYTORCH_STATIC_PROBES:=.tp)

PYTORCH_STATIC_PROBES_INCL = $(PYTORCH_STATIC_PROBES:=.h)

PYTORCH_STATIC_PROBES_SRC = $(PYTORCH_STATIC_PROBES:=.c)

$(PYTORCH_STATIC_PROBES_TP): %.tp: $(top_srcdir)/utils/gen_custom_probes.rb $(srcdir)/pytorch_events.yaml $(top_srcdir)/utils/gen_probe_base.rb
	$(RUBY) $< $(srcdir)/pytorch_events.yaml lttng_ust_$* > $@

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

# libTracerPytorch.so is meant to be LD_PRELOAD-ed onto a process that already
# has the real libtorch_cpu.so/libc10.so loaded (e.g. via `python -c "import
# torch"`), the same way the POC does it. So it only needs to *compile*
# against torch's headers here; the two dummy libraries below exist purely to
# satisfy the linker's -ltorch_cpu/-lc10 at build time. They are never
# installed or shipped: removed right after the link (all-local,
# install-exec-hook) and again on `make clean` (clean-local) as a safety net.
DUMMY_TORCH_LIBS = \
	dummy_libs/libtorch_cpu.so \
	dummy_libs/libc10.so

dummy_libs/libtorch_cpu.so dummy_libs/libc10.so:
	$(MKDIR_P) dummy_libs
	$(CXX) -shared -o $@ -x c++ /dev/null

all-local: libTracerPytorch.la
	-$(RM) $(DUMMY_TORCH_LIBS)
	-rmdir dummy_libs

install-exec-hook:
	-$(RM) $(DUMMY_TORCH_LIBS)
	-rmdir dummy_libs

clean-local:
	-$(RM) $(DUMMY_TORCH_LIBS)
	-rmdir dummy_libs

lib_LTLIBRARIES = libTracerPytorch.la

# pytorch.c is only built once, into libpytorchtracepoints.la above; pull it
# in via LIBADD rather than compiling it again here (a second copy would
# clash with the first at link time: duplicate tracepoint symbols).
nodist_libTracerPytorch_la_SOURCES = \
	$(PYTORCH_STATIC_PROBES_INCL)

libTracerPytorch_la_SOURCES = \
	tracer_pytorch.cpp

libTracerPytorch_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libTracerPytorch_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(LTTNG_UST_CFLAGS)
libTracerPytorch_la_LDFLAGS = $(LTTNG_UST_LIBS) -Ldummy_libs -ltorch_cpu -lc10 -avoid-version -module
libTracerPytorch_la_LIBADD = libpytorchtracepoints.la
EXTRA_libTracerPytorch_la_DEPENDENCIES = $(DUMMY_TORCH_LIBS)
