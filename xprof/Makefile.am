.DELETE_ON_ERROR:

# I put tally_utils.h to force the generation
# It seem to me that this is an ugly hack...

BUILT_SOURCES = xprof.c tally_utils.h

bin_SCRIPTS = iprof

iprof: $(top_builddir)/utils/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl","ze"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@


tally_utils.h: tally_utils.h.erb
	SRC_DIR=$(srcdir) ${ERB} $(srcdir)/tally_utils.h.erb > $@

# Library to be packaged
lib_LTLIBRARIES = libXProf.la

# File to compiled
nodist_libXProf_la_SOURCES = \
    tally_utils.h

# File to compiled
libXProf_la_SOURCES = \
	xprof.c \
	tally_state.h \
	tally.h \
	tally.cpp \
	timeline_state.h \
	timeline.h \
	timeline.cpp 

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

EXTRA_DIST = \
	tally_utils.h.erb

CLEANFILES = \
	tally_utils.h
