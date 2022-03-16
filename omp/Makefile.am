.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif
LTTNG_FLAGS=-fPIC -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare $(WERROR) -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I../utils -I./

OMP_HDR = \
	$(srcdir)/include/omp-tools.h

MODIFIED_OMP_HDR = \
	modified_include/omp-tools.h

modified_include/omp-tools.h: $(OMP_HDR) $(srcdir)/headers.patch
	$(RM) -r modified_include/
	cp -r $(srcdir)/include/ modified_include/
	chmod -R u+w modified_include/
	cat $(srcdir)/headers.patch | patch -i - -d modified_include/ -s -p1

clean-local:
	$(RM) -r modified_include
	$(RM) -r tests

EXTRA_DIST = $(srcdir)/include 

OMP_NAMESPACES = omp_tools

OMP_EXTRACTED = $(OMP_NAMESPACES:=_api.yaml)

$(OMP_EXTRACTED): %_api.yaml: $(srcdir)/extract_%.rb $(MODIFIED_OMP_HDR) $(srcdir)/extract_base.rb
	SRC_DIR=$(srcdir) $(RUBY) $<

CLEANFILES = $(ZE_EXTRACTED)

EXTRA_DIST += \
	extract_base.rb \
	$(OMP_NAMESPACES:%=extract_%.rb)
