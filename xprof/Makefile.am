.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

bin_SCRIPTS = iprof

iprof: $(top_builddir)/xprof/xprof.rb
	cp $(top_builddir)/xprof/xprof.rb $@
	chmod a+x $@

nodist_noinst_HEADERS = xprof.sh.erb

xprof_utils.hpp: $(top_srcdir)/utils/xprof_utils.hpp
	cp $< $@

%.pb.h %.pb.cc: %.proto
	$(PROTOC) -I=$(srcdir) --cpp_out=$(builddir) $<

BTX_TALLY_GENERATED = \
	btx_sink_tally/metababel/metababel.h \
	btx_sink_tally/metababel/btx_component.h \
	btx_sink_tally/metababel/btx_component.c \
	btx_sink_tally/metababel/btx_upstream.h \
	btx_sink_tally/metababel/btx_upstream.c \
	btx_sink_tally/btx_main.c

BTX_TIMELINE_GENERATED = \
	btx_sink_timeline/metababel/metababel.h \
	btx_sink_timeline/metababel/btx_component.h \
	btx_sink_timeline/metababel/btx_component.c \
	btx_sink_timeline/metababel/btx_upstream.h \
	btx_sink_timeline/metababel/btx_upstream.c \
	btx_sink_timeline/btx_main.c

BTX_AGGREG_GENERATED = \
        btx_filter_aggreg/metababel/metababel.h \
        btx_filter_aggreg/metababel/btx_component.h \
        btx_filter_aggreg/metababel/btx_component.c \
        btx_filter_aggreg/metababel/btx_upstream.h \
        btx_filter_aggreg/metababel/btx_upstream.c \
        btx_filter_aggreg/metababel/btx_downstream.h \
        btx_filter_aggreg/metababel/btx_downstream.c \
        btx_filter_aggreg/btx_main.c

$(BTX_TIMELINE_GENERATED) &: $(srcdir)/btx_interval_model.yaml $(srcdir)/btx_timeline_params.yaml
	$(METABABEL) -u $(srcdir)/btx_interval_model.yaml -t SINK -o btx_sink_timeline -p btx_timeline -c timeline --params $(srcdir)/btx_timeline_params.yaml

$(BTX_TALLY_GENERATED) &: $(srcdir)/btx_aggreg_model.yaml $(srcdir)/btx_tally_params.yaml
	$(METABABEL) -u $(srcdir)/btx_aggreg_model.yaml -t SINK -o btx_sink_tally -p btx_tally -c tally --params $(srcdir)/btx_tally_params.yaml

$(BTX_AGGREG_GENERATED) &: $(srcdir)/btx_interval_model.yaml $(srcdir)/btx_aggreg_model.yaml $(srcdir)/btx_aggreg_params.yaml
	$(METABABEL) -u $(srcdir)/btx_interval_model.yaml -d $(srcdir)/btx_aggreg_model.yaml -t FILTER -o btx_filter_aggreg -p btx_aggreg -c aggreg --params $(srcdir)/btx_aggreg_params.yaml

BUILT_SOURCES = \
	$(BTX_TALLY_GENERATED) \
	$(BTX_TIMELINE_GENERATED) \
        $(BTX_AGGREG_GENERATED) \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# Library to be packaged
lib_LTLIBRARIES = libXTally.la libXTimeline.la libXAggreg.la

# File generated but not distributed
nodist_libXTally_la_SOURCES = \
	$(BTX_TALLY_GENERATED) \
	xprof_utils.hpp

nodist_libXTimeline_la_SOURCES = \
	$(BTX_TIMELINE_GENERATED) \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

nodist_libXAggreg_la_SOURCES = \
        $(BTX_AGGREG_GENERATED) \
	xprof_utils.hpp

# File not generated
libXTally_la_SOURCES = \
	tally_core.hpp \
	btx_tally.hpp \
	btx_tally.cpp \
	my_demangle.c \
	my_demangle.h \
	$(top_srcdir)/utils/include/json.hpp

libXTimeline_la_SOURCES = \
	btx_timeline.cpp

libXAggreg_la_SOURCES = \
	tally_core.hpp \
	btx_aggreg.cpp

# JSON: Right now we harcode it, We should use pkgtools at some point https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp

# Compiler flags
libXTally_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_sink_tally
libXTally_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXTally_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXTally_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

libXTimeline_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_sink_timeline
libXTimeline_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXTimeline_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS) $(PROTOBUF_CFLAGS)
libXTimeline_la_LDFLAGS = $(BABELTRACE2_LIBS) $(PROTOBUF_LIBS) -avoid-version -module


libXAggreg_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_aggreg
libXAggreg_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXAggreg_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXAggreg_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libXAggreg.so $(DESTDIR)$(pkglibdir)/bt2/libXAggreg.so
	$(LN) -f $(DESTDIR)$(libdir)/libXTimeline.so $(DESTDIR)$(pkglibdir)/bt2/libXTimeline.so
	$(LN) -f $(DESTDIR)$(libdir)/libXTally.so $(DESTDIR)$(pkglibdir)/bt2/libXTally.so

uninstall-hook:
	$(RM) -f $(DESTDIR)$(pkglibdir)/bt2/libXAggreg.so $(DESTDIR)$(pkglibdir)/bt2/libXTimeline.so $(DESTDIR)$(pkglibdir)/bt2/libXTally.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

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
	btx_interval_model.yaml \
    btx_aggreg_model.yaml \
	btx_timeline_params.yaml \
	btx_tally_params.yaml \
	btx_aggreg_params.yaml \
    interval_model.yaml \
	$(TRACE_FILES) \
	tests/tally.dust.erb \
	perfetto_prunned.proto \
	interval.c.erb \
	interval.h.erb

CLEANFILES = \
	iprof \
	xprof_utils.hpp \
	$(TALLY_DUST_FILES) \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc \
	$(BTX_TIMELINE_GENERATED) \
	$(BTX_TALLY_GENERATED) \
	$(BTX_AGGREG_GENERATED)
