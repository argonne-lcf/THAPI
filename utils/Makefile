all: lttng/tracepoint_gen.h xprof_utils.h

lttng/tracepoint_gen.h: tracepoint_gen.rb
	ruby tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

xprof_utils.h: xprof_utils.h.erb
	erb xprof_utils.h.erb > xprof_utils.h

clean:
	rm -f lttng/tracepoint_gen.h xprof_utils.h
