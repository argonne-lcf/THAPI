.DELETE_ON_ERROR:

OPENCL_PROBES = opencl_tracepoints opencl_profiling opencl_source opencl_dump opencl_arguments opencl_build opencl_devices
OPENCL_PROBES_OBJS = $(OPENCL_PROBES:=.o)
OPENCL_PROBES_INCL = $(OPENCL_PROBES:=.h)
OPENCL_PROBES_SRC = $(OPENCL_PROBES:=.c)
ML_FILES = opencl_meta_parameters.yaml supported_extensions.yaml supported_enums.yaml

.PHONY: all
all: tracer_opencl.so opencl_model.yaml babeltrace_cl.so clprof.so

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-sign-compare -Werror -O3 -I./ -I../utils/ -I./include/

cl.xml.patched: cl.xml cl.xml.patch
	patch cl.xml cl.xml.patch -o cl.xml.patched

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

tracer_opencl.h: gen_opencl_header.rb
	ruby gen_opencl_header.rb > $@

opencl_model.yaml: gen_opencl_model.rb opencl_model.rb $(ML_FILES) cl.xml.patched opencl_wrapper_events.yaml opencl_tracepoints.rb
	ruby gen_opencl_model.rb > $@

opencl_tracepoints.tp: gen_opencl_probes.rb opencl_model.rb tracer_opencl.h $(ML_FILES) cl.xml.patched opencl_tracepoints.rb opencl_wrapper_events.yaml
	ruby gen_opencl_probes.rb > $@

opencl_build.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_build > $@

opencl_arguments.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_arguments > $@

opencl_dump.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_dump > $@

opencl_profiling.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_profiling > $@

opencl_source.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_source > $@

opencl_devices.tp: gen_opencl_custom_probes.rb opencl_events.yaml
	ruby gen_opencl_custom_probes.rb lttng_ust_opencl_devices > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_opencl.c: gen_opencl.rb opencl_model.rb $(ML_FILES) cl.xml.patched tracer_opencl_helpers.include.c
	ruby gen_opencl.rb > tracer_opencl.c

tracer_opencl.so: ../utils/lttng/tracepoint_gen.h tracer_opencl.c $(OPENCL_PROBES_OBJS) $(OPENCL_PROBES_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./ -I../utils/ -I./include/ -o tracer_opencl.so -shared -fPIC -Wl,--version-script,tracer_opencl.map tracer_opencl.c $(OPENCL_PROBES_OBJS) -llttng-ust -ldl -lffi

babeltrace_cl_callbacks.h: gen_babeltrace_cl_callbacks.rb opencl_model.yaml
	ruby gen_babeltrace_cl_callbacks.rb > babeltrace_cl_callbacks.h

babeltrace_cl_dispatchers.c: gen_babeltrace_cl_dispatchers.rb opencl_model.yaml
	ruby gen_babeltrace_cl_dispatchers.rb > babeltrace_cl_dispatchers.c

# Dust 
dust.c: dust.c.erb gen_clprof.rb opencl_model.yaml
	ruby gen_clprof.rb dust 

dust.so: dust.c
	gcc dust.c -fPIC -c $(pkg-config --cflags babeltrace2)
	ld dust.o -o dust.so -shared $(pkg-config --libs babeltrace2)


# Iprof
clprof.c: clprof_callbacks.cpp.erb clprof_callbacks.h.erb clprof.c.erb gen_clprof.rb opencl_model.yaml babeltrace_cl_callbacks.h tracer_opencl.h
	ruby gen_clprof.rb production

clprof_callbacks.cpp clprof_callbacks.h: clprof.c
	@if test -f $@; then \
	  touch $@; \
	else \
	  rm -f clprof.c; \
	  $(MAKE) $(AM_MAKEFLAGS) clprof.c; \
	fi
	
clprof_callbacks.o: clprof_callbacks.cpp clprof_callbacks.h 
	g++ -std=c++17 -g -O3 -Wall -Wextra -Wno-unused-parameter -Werror -I./ -I../utils/ -I./include/ $(shell pkg-config --cflags babeltrace2) -o clprof_callbacks.o -fPIC -c clprof_callbacks.cpp

clprof.so: clprof.c clprof.h clprof_callbacks.o tracer_opencl.h babeltrace_cl_dispatchers.c 
	gcc -g -O3 -Wall -Wextra -Wno-unused-parameter -Werror -I./ -I../utils/ -I./include/ $(shell pkg-config --cflags babeltrace2) -o clprof.so -shared -fPIC clprof.c babeltrace_cl_dispatchers.c clprof_callbacks.o -lstdc++ $(shell pkg-config --libs babeltrace2)

# Iprof test
testing.mk: gen_clprof.rb opencl_model.yaml testing.mk.erb
	ruby gen_clprof.rb testing_makefile

.PHONY: run_clprof_test
run_clprof_test: testing.mk babeltrace_cl_dispatchers.c  babeltrace_cl_callbacks.h tracer_opencl.h
	$(MAKE) --no-print-directory -f testing.mk all_test

# Babeltrace
babeltrace_cl.so: babeltrace_cl.c babeltrace_cl.h babeltrace_cl_dispatchers.c tracer_opencl.h babeltrace_cl_callbacks.h
	gcc -g -O3 -Wall -Wextra -Wno-unused-parameter -Werror -I./ -I../utils/ -I./include/ $(shell pkg-config --cflags babeltrace2) -o babeltrace_cl.so -shared -fPIC babeltrace_cl.c babeltrace_cl_dispatchers.c $(shell pkg-config --libs babeltrace2)

clean:
	rm -f tracer_opencl.c tracer_opencl.h tracer_opencl.so dust.so babeltrace_cl.so *.tp $(OPENCL_PROBES_OBJS) $(OPENCL_PROBES_INCL) $(OPENCL_PROBES_SRC) babeltrace_cl_dispatchers.c babeltrace_cl_callbacks.h cl.xml.patched opencl_model.yaml clprof_callbacks.cpp clprof_callbacks.o clprof_callbacks.h clprof.c clprof.so
	$(MAKE) -C ../utils clean
	test -f testing.mk && $(MAKE) -s -f testing.mk clean; rm -f testing.mk
