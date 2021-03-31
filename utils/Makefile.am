noinst_HEADERS = \
	lttng/tracepoint.h \
	include/utarray.h \
	include/uthash.h \
	include/utlist.h
nodist_noinst_HEADERS = lttng/tracepoint_gen.h xprof_utils.h xprof.sh.erb

lttng/tracepoint_gen.h: $(srcdir)/tracepoint_gen.rb
	mkdir -p lttng
	$(RUBY) $(srcdir)/tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

xprof_utils.h: $(srcdir)/xprof_utils.h.erb
	$(ERB) $(srcdir)/xprof_utils.h.erb > xprof_utils.h

CLEANFILES = lttng/tracepoint_gen.h xprof_utils.h
EXTRA_DIST = \
	LTTng.rb \
	tracepoint_gen.rb \
	yaml_ast.rb \
	xprof_utils.h.erb \
	gen_dummy_trace.rb \
	bt2.rb \
	bt_plugins/dust.rb \
	bt_plugins/comparator.rb \
	xprof_utils.h.erb \
	gen_babeltrace_dispatchers.rb
