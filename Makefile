all: tracer.so

opencl_tracepoints.tp: gen_opencl_probes.rb opencl_model.rb
	ruby gen_opencl_probes.rb > opencl_tracepoints.tp

opencl_tracepoints.o: opencl_tracepoints.tp
	CFLAGS="-fPIC -O3 -I./" lttng-gen-tp opencl_tracepoints.tp

tracer.c: gen.rb
	ruby gen.rb > tracer.c

tracer.so: tracer.c opencl_tracepoints.o
	gcc -O3 -I./ -o tracer.so -shared -fPIC -Wl,--version-script,tracer.map tracer.c opencl_tracepoints.o -llttng-ust -ldl

clean:
	rm -f tracer.c tracer.so opencl_tracepoints.tp opencl_tracepoints.o opencl_tracepoints.c opencl_tracepoints.h
