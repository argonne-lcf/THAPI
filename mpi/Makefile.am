.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

MPI_HDR = \
	$(srcdir)/include/mpi.h

MODIFIED_MPI_HDR = \
	modified_include/mpi.h

modified_include/mpi.h: $(MPI_HDR)
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/

clean-local:
	$(RM) -r modified_include

EXTRA_DIST = $(srcdir)/include

MPI_NAMESPACES = mpi

MPI_EXTRACTED = $(MPI_NAMESPACES:=_api.yaml)

$(MPI_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(MODIFIED_MPI_HDR) $(srcdir)/extract_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $<

CLEANFILES = $(MPI_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	$(MPI_NAMESPACES:%=extract_%.rb)

MPI_MODEL = \
	$(srcdir)/mpi_model.rb \
	$(srcdir)/mpi_meta_parameters.yaml \
	$(srcdir)/gen_probe_base.rb \
	$(srcdir)/mpi_events.yaml \
	$(top_srcdir)/utils/yaml_ast.rb \
	$(top_srcdir)/utils/yaml_ast_lttng.rb \
	$(top_srcdir)/utils/meta_parameters.rb \
	$(top_srcdir)/utils/LTTng.rb \
	$(MPI_EXTRACTED)

mpi_babeltrace_model.yaml: $(srcdir)/gen_babeltrace_mpi_model.rb $(MPI_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

CLEANFILES += \
	mpi_babeltrace_model.yaml

EXTRA_DIST += \
	mpi_model.rb \
	gen_mpi_library_base.rb \
	gen_probe_base.rb \
	mpi_events.yaml \
	gen_babeltrace_mpi_model.rb \
	$(srcdir)/mpi_meta_parameters.yaml

MPI_PROBES = mpi_tracepoints
MPI_PROBES_TP = $(MPI_PROBES:=.tp)
MPI_PROBES_INCL = $(MPI_PROBES:=.h)
MPI_PROBES_SRC = $(MPI_PROBES:=.c)

$(MPI_PROBES_TP): %.tp: $(srcdir)/gen_%.rb $(MPI_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

%.h %.c: %.tp
	$(LTTNG_GEN_TP) $< -o $*.c -o $*.h

CLEANFILES += \
	$(MPI_PROBES_TP) \
	$(MPI_PROBES_INCL) \
	$(MPI_PROBES_SRC)

EXTRA_DIST += \
	gen_mpi_tracepoints.rb

BUILT_SOURCES = \
        $(MODIFIED_MPI_HDR) \
        $(MPI_PROBES_INCL)

tracer_mpi.c: $(srcdir)/gen_mpi.rb $(MPI_MODEL) $(MPI_PROBES_INCL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

EXTRA_DIST += \
	gen_mpi.rb

CLEANFILES += \
	tracer_mpi.c

bin_SCRIPTS = \
	tracer_mpi.sh

noinst_LTLIBRARIES = \
	libmpitracepoints.la

nodist_libmpitracepoints_la_SOURCES = \
	$(MPI_PROBES_INCL) \
	$(MPI_PROBES_SRC)

libmpitracepoints_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./
libmpitracepoints_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) $(LTTNG_UST_CFLAGS)
libmpitracepoints_la_LDFLAGS = $(LTTNG_UST_LIBS)

nodist_libTracerMPI_la_SOURCES = \
	$(MPI_PROBES_INCL) \
	tracer_mpi.c

libTracerMPI_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(top_srcdir)/sampling -I$(srcdir)/include -I./utils -I./
libTracerMPI_la_CFLAGS = -Wall -Wextra $(WERROR) $(LIBFFI_CFLAGS) $(LTTNG_UST_CFLAGS)
libTracerMPI_la_LDFLAGS = $(LTTNG_UST_LIBS) -ldl -lpthread $(LIBFFI_LIBS) ../sampling/libThapiSampling.la
libTracerMPI_la_LIBADD = libmpitracepoints.la

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/mpi
	$(LN_S) -f $(DESTDIR)$(libdir)/libTracerMPI.so.1.0.0 $(DESTDIR)$(pkglibdir)/mpi/libmpi.so.1
	$(LN_S) -f $(DESTDIR)$(pkglibdir)/mpi/libmpi.so.1 $(DESTDIR)$(pkglibdir)/mpi/libmpi.so
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libMPIInterval.so $(DESTDIR)$(pkglibdir)/bt2/libMPIInterval.so


uninstall-hook:
	$(RM) -f $(DESTDIR)$(libdir)/libTracerMPI.so.1.0.0 $(DESTDIR)$(pkglibdir)/mpi/libmpi.so.1
	$(RM) -f $(DESTDIR)$(pkglibdir)/mpi/libmpi.so.1 $(DESTDIR)$(pkglibdir)/mpi/libmpi.so
	-rmdir $(DESTDIR)$(pkglibdir)/mpi
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libMPIInterval.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

tmplibdir = $(libdir)/tmp
install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

EXTRA_DIST += mpi.h.include

mpi_library.rb: $(srcdir)/gen_mpi_library.rb $(MPI_MODEL)
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

babeltrace_mpi_lib.rb: $(srcdir)/gen_babeltrace_mpi_lib.rb $(top_srcdir)/utils/gen_library_base.rb $(MPI_MODEL) mpi_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $< > $@

MPI_BINDINGS = \
	mpi_library.rb \
	babeltrace_mpi_lib.rb

EXTRA_DIST += \
	gen_mpi_library.rb \
	gen_babeltrace_mpi_lib.rb

CLEANFILES += \
	$(MPI_BINDINGS)

data_DATA = \
	$(MPI_BINDINGS)

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

BTX_MPI_GENERATED = \
	btx_filter_mpi/metababel/metababel.h \
	btx_filter_mpi/metababel/btx_component.h \
	btx_filter_mpi/metababel/btx_component.c \
	btx_filter_mpi/metababel/btx_upstream.h \
	btx_filter_mpi/metababel/btx_upstream.c \
	btx_filter_mpi/metababel/btx_downstream.h \
	btx_filter_mpi/metababel/btx_downstream.c \
	btx_filter_mpi/btx_main.c

EXTRA_DIST += \
	$(top_srcdir)/utils/babeltrace_to_metababel_model.rb \
	$(top_srcdir)/xprof/btx_interval_model.yaml \
	btx_mpimatching_model.yaml

btx_mpi_model.yaml: $(top_srcdir)/utils/babeltrace_to_metababel_model.rb mpi_babeltrace_model.yaml
	SRC_DIR=$(srcdir) $(RUBY) $^ > $@

$(BTX_MPI_GENERATED) &: $(top_srcdir)/xprof/btx_interval_model.yaml btx_mpimatching_model.yaml btx_mpi_model.yaml
	$(METABABEL) -u btx_mpi_model.yaml -d $(top_srcdir)/xprof/btx_interval_model.yaml -t FILTER -o btx_filter_mpi -p mpiinterval -c interval --matching $(srcdir)/btx_mpimatching_model.yaml -i mpi.h.include

CLEANFILES += \
	$(BTX_MPI_GENERATED) \
	btx_mpi_model.yaml \
	xprof_utils.hpp

BUILT_SOURCES += \
	$(BTX_MPI_GENERATED)

nodist_libMPIInterval_la_SOURCES = \
	$(BTX_MPI_GENERATED) \
	xprof_utils.hpp

libMPIInterval_la_SOURCES = \
	btx_mpiinterval_callbacks.cpp

libMPIInterval_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_mpi
libMPIInterval_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libMPIInterval_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libMPIInterval_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

lib_LTLIBRARIES = \
	libTracerMPI.la \
	libMPIInterval.la

