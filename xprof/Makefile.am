.DELETE_ON_ERROR:

if STRICT
  WERROR = -Werror
else
  WERROR =
endif

bin_SCRIPTS = \
	iprof \
	sync_daemon_fs

if FOUND_MPI
  bin_SCRIPTS += sync_daemon_mpi
endif

sync_daemon_mpi: $(srcdir)/sync_daemon_mpi.c
	$(MPICC) $(CFLAGS) $< -o $@

iprof: xprof.rb
	cp xprof.rb $@

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

BTX_STRIPPER_GENERATED = \
	btx_filter_stripper/metababel/metababel.h \
	btx_filter_stripper/metababel/btx_component.h \
	btx_filter_stripper/metababel/btx_component.c \
	btx_filter_stripper/metababel/btx_upstream.h \
	btx_filter_stripper/metababel/btx_upstream.c \
	btx_filter_stripper/metababel/btx_downstream.h \
	btx_filter_stripper/metababel/btx_downstream.c \
	btx_filter_stripper/btx_main.c

$(BTX_TIMELINE_GENERATED) &: $(srcdir)/btx_interval_model.yaml $(srcdir)/btx_timeline_params.yaml
	$(METABABEL) -u $(srcdir)/btx_interval_model.yaml -t SINK -o btx_sink_timeline -p btx_timeline -c timeline --params $(srcdir)/btx_timeline_params.yaml

$(BTX_TALLY_GENERATED) &: $(srcdir)/btx_aggreg_model.yaml $(srcdir)/btx_tally_params.yaml
	$(METABABEL) -u $(srcdir)/btx_aggreg_model.yaml -t SINK -o btx_sink_tally -p btx_tally -c tally --params $(srcdir)/btx_tally_params.yaml

$(BTX_AGGREG_GENERATED) &: $(srcdir)/btx_interval_model.yaml $(srcdir)/btx_aggreg_model.yaml $(srcdir)/btx_aggreg_params.yaml
	$(METABABEL) -u $(srcdir)/btx_interval_model.yaml -d $(srcdir)/btx_aggreg_model.yaml -t FILTER -o btx_filter_aggreg -p btx_aggreg -c aggreg --params $(srcdir)/btx_aggreg_params.yaml

$(BTX_STRIPPER_GENERATED) &:
	$(METABABEL) -t FILTER -o btx_filter_stripper -p btx_stripper -c stripper --enable-callbacks on_downstream

btx_stripper_callbacks.cpp:
	$(METABABEL) --display-shared-callback stripper > $@

BUILT_SOURCES = \
	$(BTX_TALLY_GENERATED) \
	$(BTX_TIMELINE_GENERATED) \
	$(BTX_AGGREG_GENERATED) \
	$(BTX_STRIPPER_GENERATED) \
	btx_stripper_callbacks.cpp \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc

# Library to be packaged
lib_LTLIBRARIES = libXTally.la libXTimeline.la libXAggreg.la libXStripper.la

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

nodist_libXStripper_la_SOURCES = \
	$(BTX_STRIPPER_GENERATED) \
	btx_stripper_callbacks.cpp

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

libXStripper_la_CPPFLAGS = -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./ -I./btx_filter_stripper
libXStripper_la_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXStripper_la_CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter $(WERROR) -fno-fast-math $(BABELTRACE2_CFLAGS)
libXStripper_la_LDFLAGS = $(BABELTRACE2_LIBS) -avoid-version -module

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/bt2
	$(LN) -f $(DESTDIR)$(libdir)/libXAggreg.so $(DESTDIR)$(pkglibdir)/bt2/libXAggreg.so
	$(LN) -f $(DESTDIR)$(libdir)/libXTimeline.so $(DESTDIR)$(pkglibdir)/bt2/libXTimeline.so
	$(LN) -f $(DESTDIR)$(libdir)/libXTally.so $(DESTDIR)$(pkglibdir)/bt2/libXTally.so
	$(LN) -f $(DESTDIR)$(libdir)/libXStripper.so $(DESTDIR)$(pkglibdir)/bt2/libXStripper.so

uninstall-hook:
	$(RM) $(DESTDIR)$(pkglibdir)/bt2/libXAggreg.so $(DESTDIR)$(pkglibdir)/bt2/libXTimeline.so $(DESTDIR)$(pkglibdir)/bt2/libXTally.so $(DESTDIR)$(pkglibdir)/bt2/libXStripper.so
	-rmdir $(DESTDIR)$(pkglibdir)/bt2

# Cannot use check_LTLIBRARIES because we need the shared version of those
# Thanks Vincent Danjean
#   noinst_LTLIBRARIES would be the correct thing but then libtool
#   only built non shared version :-( So, declaring the libs as
#   pkglib_LTLIBRARIES and using an install hook to remove them.
tmplibdir = $(libdir)/tmp
install-data-hook:
	$(RM) -r $(DESTDIR)$(tmplibdir)

BTX_INTERVAL_GENERATED_SOURCE_TEST = \
	btx_source_interval_test/metababel/metababel.h \
	btx_source_interval_test/metababel/btx_component.h \
	btx_source_interval_test/metababel/btx_component.c \
	btx_source_interval_test/metababel/btx_downstream.h \
	btx_source_interval_test/metababel/btx_downstream.c \
	btx_source_interval_test/btx_main.c

$(BTX_INTERVAL_GENERATED_SOURCE_TEST) &: $(srcdir)/btx_interval_model.yaml
	$(METABABEL) -d $(srcdir)/btx_interval_model.yaml -t SOURCE  -o btx_source_interval_test -p intervaltest -c test

noinst_LTLIBRARIES = libtestintervalsource.la
nodist_libtestintervalsource_la_SOURCES = $(BTX_INTERVAL_GENERATED_SOURCE_TEST)
libtestintervalsource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_interval_test/
libtestintervalsource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)

BTX_AGGREG_GENERATED_SOURCE_TEST = \
	btx_source_aggreg_test/metababel/metababel.h \
	btx_source_aggreg_test/metababel/btx_component.h \
	btx_source_aggreg_test/metababel/btx_component.c \
	btx_source_aggreg_test/metababel/btx_downstream.h \
	btx_source_aggreg_test/metababel/btx_downstream.c \
	btx_source_aggreg_test/btx_main.c

$(BTX_AGGREG_GENERATED_SOURCE_TEST) &: $(srcdir)/btx_aggreg_model.yaml
	$(METABABEL) -d $(srcdir)/btx_aggreg_model.yaml -t SOURCE  -o btx_source_aggreg_test -p aggregtest -c test

noinst_LTLIBRARIES += libtestaggregsource.la
nodist_libtestaggregsource_la_SOURCES = $(BTX_AGGREG_GENERATED_SOURCE_TEST)
libtestaggregsource_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/utils/include -I$(srcdir)/include -I./btx_source_aggreg_test/
libtestaggregsource_la_CFLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter $(WERROR) $(BABELTRACE2_CFLAGS)


TRACE_COMMON = \
	tests/interval_to_aggreg.thapi_interval_text_pretty \
	tests/aggreg_to_tally.thapi_aggreg_text_pretty

TRACE_OUT = \
        tests/interval_to_aggreg.bt_text_pretty \
        tests/aggreg_to_tally.bt_text_pretty

TESTS = \
	$(TRACE_COMMON)

TEST_EXTENSIONS = .thapi_interval_text_pretty .thapi_aggreg_text_pretty
THAPI_INTERVAL_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_INTERVAL_TEXT_PRETTY_LOG_FLAGS = xprof interval btx_source_interval_test/ .libs/libtestintervalsource.a
THAPI_AGGREG_TEXT_PRETTY_LOG_COMPILER = $(top_builddir)/utils/test_wrapper_thapi_text_pretty.sh
AM_THAPI_AGGREG_TEXT_PRETTY_LOG_FLAGS = xprof aggreg btx_source_aggreg_test/ .libs/libtestaggregsource.a

check_DATA = \
	$(TRACE_COMMON)

EXTRA_DIST = \
	btx_interval_model.yaml \
	btx_aggreg_model.yaml \
	btx_timeline_params.yaml \
	btx_tally_params.yaml \
	btx_aggreg_params.yaml \
	$(TRACE_COMMON)\
	$(TRACE_OUT) \
	perfetto_prunned.proto \
	sync_daemon_fs

CLEANFILES = \
	iprof \
	xprof_utils.hpp \
	perfetto_prunned.pb.h \
	perfetto_prunned.pb.cc \
	btx_stripper_callbacks.cpp \
	$(BTX_TIMELINE_GENERATED) \
	$(BTX_TALLY_GENERATED) \
	$(BTX_AGGREG_GENERATED) \
	$(BTX_STRIPPER_GENERATED) \
	$(BTX_INTERVAL_GENERATED_SOURCE_TEST) \
	$(BTX_AGGREG_GENERATED_SOURCE_TEST)

if FOUND_MPI
  CLEANFILES += sync_daemon_mpi
endif
