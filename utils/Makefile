all: lttng/tracepoint_gen.h

lttng/tracepoint_gen.h: tracepoint_gen.rb
	ruby tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

clean:
	rm -f lttng/tracepoint_gen.h
