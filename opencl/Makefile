.DELETE_ON_ERROR:

OPENCL_PROBES = opencl_tracepoints opencl_profiling opencl_source opencl_dump opencl_arguments opencl_build
OPENCL_PROBES_OBJS = $(OPENCL_PROBES:=.o)
OPENCL_PROBES_INCL = $(OPENCL_PROBES:=.h)
OPENCL_PROBES_SRC = $(OPENCL_PROBES:=.c)
ML_FILES = opencl_meta_parameters.yaml supported_extensions.yaml supported_enums.yaml cl.xml

all: tracer_opencl.so

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Werror -O3 -I./ -I../utils/

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

tracer_opencl.h: gen_opencl_header.rb
	ruby gen_opencl_header.rb > tracer_opencl.h

opencl_tracepoints.tp: gen_opencl_probes.rb opencl_model.rb tracer_opencl.h $(ML_FILES) opencl_callbacks.tp.include
	ruby gen_opencl_probes.rb > opencl_tracepoints.tp

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_opencl.c: gen_opencl.rb opencl_model.rb $(ML_FILES) tracer_opencl_helpers.include.c
	ruby gen_opencl.rb > tracer_opencl.c

tracer_opencl.so: ../utils/lttng/tracepoint_gen.h tracer_opencl.c $(OPENCL_PROBES_OBJS) $(OPENCL_PROBES_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./ -I../utils/ -o tracer_opencl.so -shared -fPIC -Wl,--version-script,tracer_opencl.map tracer_opencl.c $(OPENCL_PROBES_OBJS) -llttng-ust -ldl -lffi

clean:
	rm -f tracer_opencl.c tracer_opencl.h tracer_opencl.so opencl_tracepoints.tp $(OPENCL_PROBES_OBJS) $(OPENCL_PROBES_INCL) $(OPENCL_PROBES_SRC)
	$(MAKE) -C ../utils clean
