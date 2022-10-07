.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

bin_SCRIPTS = iprof

iprof: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl","ze","cuda","omp"] $< > $@
	chmod a+x $@

nodist_noinst_HEADERS = xprof.sh.erb

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

%.pb.h %.pb.cc: %.proto
	$(PROTOC) -I=$(srcdir) --cpp_out=$(builddir) $<

BUILT_SOURCES = \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# Library to be packaged
lib_LTLIBRARIES = libXProf.la

# File to compiled
nodist_libXProf_la_SOURCES = \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# File to compiled
libXProf_la_SOURCES = \
	xprof.c \
	tally.hpp \
	tally_utils.hpp \
	tally.h \
	tally.cpp \
	timeline.hpp \
	timeline.h \
	timeline.cpp \
	my_demangle.c \
	my_demangle.h \
	$(top_srcdir)/utils/include/json.hpp

# Compiler flags
libXProf_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I/home/tapplencourt/tmp/proto/protobuf-3.20.1/ici/include/
libXProf_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libXProf_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS) $(PROTOBUF_CFLAGS)
libXProf_la_LDFLAGS = $(BABELTRACE2_LIBS) $(PROTOBUF_LIBS) -avoid-version -module

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
	$(ERB) namespace="$@" $< > $@

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
	interval.c.erb \
	interval.h.erb \
	interval_model.yaml \
	$(TRACE_FILES) \
	tests/tally.dust.erb \
	perfetto_prunned.proto

CLEANFILES = \
	iprof \
	xprof_utils.hpp \
	$(TALLY_DUST_FILES) \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc
