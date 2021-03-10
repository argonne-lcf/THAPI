noinst_HEADERS = lttng/tracepoint.h utarray.h uthash.h utlist.h xprof_utils.hpp
nodist_noinst_HEADERS = lttng/tracepoint_gen.h

lttng/tracepoint_gen.h: $(srcdir)/tracepoint_gen.rb
	mkdir -p lttng
	$(RUBY) $(srcdir)/tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

CLEANFILES = lttng/tracepoint_gen.h
EXTRA_DIST = LTTng.rb tracepoint_gen.rb yaml_ast.rb xprof_utils.cpp
