all: tracer.so

opencl_tracepoints.tp: gen_opencl_probes.rb opencl_model.rb
	ruby gen_opencl_probes.rb > opencl_tracepoints.tp

opencl_tracepoints.o: opencl_tracepoints.tp lttng/tracepoint_gen.h
	CFLAGS="-fPIC -Wall -O3 -I./" lttng-gen-tp opencl_tracepoints.tp

lttng/tracepoint_gen.h: tracepoint_gen.rb
	ruby tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

tracer.c: gen.rb opencl_model.rb
	ruby gen.rb > tracer.c

tracer.so: tracer.c opencl_tracepoints.o
	gcc -O3 -Wall -I./ -o tracer.so -shared -fPIC -Wl,--version-script,tracer.map tracer.c opencl_tracepoints.o -llttng-ust -ldl

clean:
	rm -f tracer.c tracer.so opencl_tracepoints.tp opencl_tracepoints.o opencl_tracepoints.c opencl_tracepoints.h lttng/tracepoint_gen.h
