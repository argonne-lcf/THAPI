all: tracer_opencl.so

LTTNG_FLAGS=-fPIC -g -Wall -Werror -O3 -I./ -I../utils/

utils:
	$(MAKE) -C ../utils

tracer_opencl.h: gen_opencl_header.rb
	ruby gen_opencl_header.rb > tracer_opencl.h

opencl_tracepoints.tp: gen_opencl_probes.rb opencl_model.rb tracer_opencl.h opencl_meta_parameters.yaml
	ruby gen_opencl_probes.rb > opencl_tracepoints.tp

opencl_tracepoints.o: opencl_tracepoints.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp opencl_tracepoints.tp

opencl_profiling.o: opencl_profiling.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp opencl_profiling.tp

opencl_source.o: opencl_source.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp opencl_source.tp

opencl_dump.o: opencl_dump.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp opencl_dump.tp

lttng/tracepoint_gen.h: tracepoint_gen.rb
	ruby tracepoint_gen.rb 25 > lttng/tracepoint_gen.h

tracer_opencl.c: gen_opencl.rb opencl_model.rb opencl_meta_parameters.yaml tracer_opencl_helpers.include.c
	ruby gen_opencl.rb > tracer_opencl.c

tracer_opencl.so: utils tracer_opencl.c opencl_tracepoints.o opencl_profiling.o opencl_source.o opencl_dump.o
	gcc -g -O3 -Wall -pedantic -Wextra -Werror -I./ -I../utils/ -o tracer_opencl.so -shared -fPIC -Wl,--version-script,tracer_opencl.map tracer_opencl.c opencl_tracepoints.o opencl_profiling.o opencl_source.o opencl_dump.o -llttng-ust -ldl -lffi

clean:
	rm -f tracer_opencl.c tracer_opencl.h tracer_opencl.so opencl_tracepoints.tp opencl_profiling.o opencl_source.o opencl_tracepoints.o opencl_dump.o opencl_tracepoints.c opencl_profiling.c opencl_source.c opencl_dump.c opencl_tracepoints.h opencl_profiling.h opencl_source.h opencl_dump.h
	$(MAKE) -C ../utils clean
