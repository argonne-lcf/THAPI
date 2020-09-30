noinst_HEADERS = lttng/tracepoint.h utarray.h uthash.h utlist.h
nodist_noinst_HEADERS = lttng/tracepoint_gen.h xprof_utils.h

lttng/tracepoint_gen.h: $(srcdir)/tracepoint_gen.rb
	mkdir -p lttng
	$(RUBY) $(srcdir)/tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

xprof_utils.h: $(srcdir)/xprof_utils.h.erb
	$(ERB) $(srcdir)/xprof_utils.h.erb > xprof_utils.h

CLEANFILES = lttng/tracepoint_gen.h xprof_utils.h
EXTRA_DIST = LTTng.rb tracepoint_gen.rb xprof_utils.h.erb
