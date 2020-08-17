.DELETE_ON_ERROR:
.SUFFIXES:

ZE_PROBES = ze_tracepoints zet_tracepoints
ZE_TP = $(ZE_PROBES:=.tp)
ZE_OBJS = $(ZE_PROBES:=.o)
ZE_INCL = $(ZE_PROBES:=.h)
ZE_SRC = $(ZE_PROBES:=.c)

ZE_STATIC_PROBES = ze_profiling
ZE_STATIC_TP = $(ZE_STATIC_PROBES:=.tp)
ZE_STATIC_OBJS = $(ZE_STATIC_PROBES:=.o)
ZE_STATIC_INCL = $(ZE_STATIC_PROBES:=.h)
ZE_STATIC_SRC = $(ZE_STATIC_PROBES:=.c)

ZE_EXTRACT = extract_base.rb modified_include
ZE_EXTRACTED = ze_api.yaml zet_api.yaml zes_api.yaml
ZE_MODEL = ze_model.rb ../utils/yaml_ast.rb ze_meta_parameters.yaml zet_meta_parameters.yaml $(ZE_EXTRACTED)

ZE_BINDINGS = ze_bindings.rb ze_refinements.rb

.SECONDARY: $(ZE_TP)

all: tracer_ze.so ze_library.rb babeltrace_ze_lib.rb $(ZE_BINDINGS)

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-nonnull -Werror -O3 -I./ -I./include/ -I../utils/

modified_include: include headers.patch
	cp -r include/ modified_include/
	patch -i ../headers.patch -d modified_include/ -s -p1

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

ze_api.yaml: $(shell find modified_include/ -type f) $(ZE_EXTRACT) extract_ze.rb
	ruby extract_ze.rb

zet_api.yaml: $(shell find modified_include/ -type f) $(ZE_EXTRACT) extract_zet.rb
	ruby extract_zet.rb

zes_api.yaml: $(shell find modified_include/ -type f) $(ZE_EXTRACT) extract_zes.rb
	ruby extract_zes.rb

ze_profiling.tp: gen_ze_custom_probes.rb ../utils/LTTng.rb $(ZE_MODEL)
	ruby gen_ze_custom_probes.rb lttng_ust_ze_profiling > $@

%.tp: %.rb $(ZE_MODEL) gen_probe_base.rb
	ruby $*.rb > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_ze.c: gen_ze.rb tracer_ze_helpers.include.c $(ZE_MODEL)
	ruby $< > $@

tracer_ze.so: ../utils/lttng/tracepoint_gen.h tracer_ze.c $(ZE_OBJS) $(ZE_INCL) $(ZE_STATIC_OBJS) $(ZE_STATIC_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./include/ -I./ -I../utils/ -o tracer_ze.so -shared -fPIC tracer_ze.c $(ZE_OBJS) $(ZE_STATIC_OBJS) -llttng-ust -ldl -lffi

ze_library.rb: gen_ze_library.rb gen_ze_library_base.rb gen_probe_base.rb $(ZE_MODEL)
	ruby $< > $@

babeltrace_ze_lib.rb: gen_babeltrace_ze_lib.rb gen_ze_library_base.rb gen_probe_base.rb $(ZE_MODEL)
	ruby $< > $@

ze_refinements.rb: gen_ze_refinements.rb gen_ze_library_base.rb gen_probe_base.rb $(ZE_MODEL)
	ruby $< > $@

ze_bindings.rb: gen_ze_bindings.rb gen_ze_library_base.rb gen_probe_base.rb $(ZE_MODEL)
	ruby $< > $@

# objcopy --prefix-symbols= on ze_api.a content should allow a wrapper creation

clean:
	rm -f \
		ze_api.yaml zet_api.yaml\
		$(ZE_TP) $(ZE_OBJS) $(ZE_INCL) $(ZE_SRC)\
		$(ZE_STATIC_TP) $(ZE_STATIC_OBJS) $(ZE_STATIC_INCL) $(ZE_STATIC_SRC)\
		$(ZE_BINDINGS)\
		tracer_ze.c tracer_ze.so ze_library.rb babeltrace_ze_lib.rb
	rm -fr modified_include
	$(MAKE) -C ../utils clean
