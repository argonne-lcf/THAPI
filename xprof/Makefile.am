.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

bin_SCRIPTS = iprof

iprof: $(top_builddir)/xprof/xprof.sh.erb
	$(ERB) -T 1 languages="cl","ze","cuda","omp","hip" $< > $@
	chmod a+x $@

nodist_noinst_HEADERS = xprof.sh.erb

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

%.pb.h %.pb.cc: %.proto
	$(PROTOC) -I=$(srcdir) --cpp_out=$(builddir) $<

btx_timeline_upstream_model.yaml btx_tally_upstream_model.yaml &: btx_interval_model.yaml
	cp $< $(builddir)/btx_timeline_upstream_model.yaml
	cp $< $(builddir)/btx_tally_upstream_model.yaml

%/metababel/metababel.h %/metababel/btx_component.h %/metababel/btx_component.c %/metababel/btx_upstream.h %/metababel/btx_upstream.c %/btx_main.c: %_upstream_model.yaml %_params.yaml
	$(METABABEL) -u $(builddir)/$*_upstream_model.yaml -t SINK -o $* -p $* -c $* --params $(srcdir)/$*_params.yaml

BTX_TALLY_GENERATED = \
	btx_tally/metababel/metababel.h \
	btx_tally/metababel/btx_component.h \
	btx_tally/metababel/btx_component.c \
	btx_tally/metababel/btx_upstream.h \
	btx_tally/metababel/btx_upstream.c \
	btx_tally/btx_main.c

BTX_TIMELINE_GENERATED = \
	btx_timeline/metababel/metababel.h \
	btx_timeline/metababel/btx_component.h \
	btx_timeline/metababel/btx_component.c \
	btx_timeline/metababel/btx_upstream.h \
	btx_timeline/metababel/btx_upstream.c \
	btx_timeline/btx_main.c

BUILT_SOURCES = \
	$(BTX_TALLY_GENERATED) \
	$(BTX_TIMELINE_GENERATED) \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# Library to be packaged
lib_LTLIBRARIES = libXTally.la libXTimeline.la

# File generated but not distributed
nodist_libXTally_la_SOURCES = \
	$(BTX_TALLY_GENERATED) \
	xprof_utils.hpp

nodist_libXTimeline_la_SOURCES = \
 	$(BTX_TIMELINE_GENERATED) \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# File generated
libXTally_la_SOURCES = \
	tally.hpp \
	tally.cpp \
	my_demangle.c \
	my_demangle.h \
	$(top_srcdir)/utils/include/json.hpp

libXTimeline_la_SOURCES = \
	timeline.cpp

# JSON: Right now we harcode it, We should use pkgtools at some point https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp

# Compiler flags
libXTally_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_tally
libXTally_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libXTally_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libXTally_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

libXTimeline_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_timeline
libXTimeline_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)
libXTimeline_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS) $(PROTOBUF_CFLAGS)
libXTimeline_la_LDFLAGS = $(BABELTRACE2_LIBS) $(PROTOBUF_LIBS) -avoid-version -module

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
	btx_interval_model.yaml \
	interval_model.yaml \
	btx_timeline_params.yaml \
	btx_tally_params.yaml \
	$(TRACE_FILES) \
	tests/tally.dust.erb \
	perfetto_prunned.proto

CLEANFILES = \
	iprof \
	xprof_utils.hpp \
	$(TALLY_DUST_FILES) \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc \
	btx_timeline_upstream_model.yaml \
	btx_tally_upstream_model.yaml \
	$(BTX_TIMELINE_GENERATED) \
	$(BTX_TALLY_GENERATED)
