.DELETE_ON_ERROR:
.SUFFIXES:

ZE_PROBES = ze_tracepoints zet_tracepoints
ZE_TP = $(ZE_PROBES:=.tp)
ZE_OBJS = $(ZE_PROBES:=.o)
ZE_INCL = $(ZE_PROBES:=.h)
ZE_SRC = $(ZE_PROBES:=.c)

.SECONDARY: $(ZE_TP)

all: tracer_ze.so

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Werror -O3 -I./ -I./include/core -I./include/tools -I../utils/

modified_include: include headers.patch
	cp -r include/ modified_include/
	patch -i ../headers.patch -d modified_include/ -s -p1

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

ze_api.yaml: $(shell find include/core -type f) extract_base.rb extract_ze.rb modified_include
	ruby extract_ze.rb

zet_api.yaml: $(shell find include/core include/tools -type f) extract_base.rb extract_zet.rb modified_include
	ruby extract_zet.rb

%.tp: %.rb ze_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb gen_probe_base.rb ze_meta_parameters.yaml zet_meta_parameters.yaml
	ruby $*.rb > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_ze.c: gen_ze.rb tracer_ze_helpers.include.c ze_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb ze_meta_parameters.yaml zet_meta_parameters.yaml
	ruby $< > $@

tracer_ze.so: ../utils/lttng/tracepoint_gen.h tracer_ze.c $(ZE_OBJS) $(ZE_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./include/core -I./include/tools -I./ -I../utils/ -o tracer_ze.so -shared -fPIC tracer_ze.c $(ZE_OBJS) -llttng-ust -ldl -lffi

# objcopy --prefix-symbols= on ze_api.a content should allow a wrapper creation

clean:
	rm -f ze_api.yaml zet_api.yaml $(ZE_TP) $(ZE_OBJS) $(ZE_INCL) $(ZE_SRC) tracer_ze.c tracer_ze.so
	rm -fr modified_include
	$(MAKE) -C ../utils clean
