.DELETE_ON_ERROR:

bin_SCRIPTS = iprof 

iprof: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl","ze","cuda"] $<  > $@
	chmod a+x $@

nodist_noinst_HEADERS = xprof.sh.erb

tally_utils.hpp: tally_utils.hpp.erb
	SRC_DIR=$(srcdir) ${ERB} $(srcdir)/tally_utils.hpp.erb > $@

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

BUILT_SOURCES = \
	tally_utils.hpp \
	xprof_utils.hpp

# Library to be packaged
lib_LTLIBRARIES = libXProf.la

# File to compiled
nodist_libXProf_la_SOURCES = \
	tally_utils.hpp \
	xprof_utils.hpp

# File to compiled
libXProf_la_SOURCES = \
	xprof.c \
	tally.hpp \
	tally.h \
	tally.cpp \
	timeline.hpp \
	timeline.h \
	timeline.cpp \
	my_demangle.c \
	my_demangle.h \
	json.hpp # Right now we harcode it. We should use pkgtools at some point https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp

# Compiler flags
libXProf_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(srcdir)/include -I./
libXProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libXProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror $(BABELTRACE2_CFLAGS)
libXProf_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp
install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

TALLY_DUST_FILES = \
	tests/interval_profiling_normal.dust

$(TALLY_DUST_FILES): tests/%.dust: tests/tally.dust.erb
	mkdir -p tests/
	$(ERB) namespace="$@" $(srcdir)/tests/tally.dust.erb > $@

TESTS = \
	$(TALLY_DUST_FILES)

TRACE_FILES = \
	tests/interval_profiling_normal.yaml

TEST_EXTENSIONS = .dust
DUST_LOG_COMPILER = ./test_wrapper.sh
check_DATA = \
	test_wrapper.sh

data_DATA = \
	$(TRACE_FILES)

EXTRA_DIST = \
	tally_utils.hpp.erb \
	interval.c.erb \
	interval.h.erb \
	interval_model.yaml \
	$(TRACE_FILES) \
	tests/tally.dust.erb 

CLEANFILES = \
	iprof \
	tally_utils.hpp \
	xprof_utils.hpp \
	$(TALLY_DUST_FILES)
