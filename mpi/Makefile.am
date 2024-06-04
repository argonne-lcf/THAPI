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
	$(top_srcdir)/utils/yaml_ast.rb \
	$(top_srcdir)/utils/yaml_ast_lttng.rb \
	$(top_srcdir)/utils/meta_parameters.rb \
	$(top_srcdir)/utils/LTTng.rb \
	$(MPI_EXTRACTED)

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
