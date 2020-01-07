.DELETE_ON_ERROR:

ZE_PROBES = ze_tracepoints zex_tracepoints zet_tracepoints
ZE_TP = $(ZE_PROBES:=.tp)
ZE_OBJS = $(ZE_PROBES:=.o)
ZE_INCL = $(ZE_PROBES:=.h)
ZE_SRC = $(ZE_PROBES:=.c)

all: tracer_ze.so

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Werror -O3 -I./ -I./include/core -I./include/experimental -I./include/tools -I../utils/

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

ze_api.yaml: $(shell find include/core -type f) extract_base.rb extract_ze.rb
	ruby extract_ze.rb

zex_api.yaml: $(shell find include/core include/experimental -type f) extract_base.rb extract_zex.rb
	ruby extract_zex.rb

zet_api.yaml: $(shell find include/core include/experimental include/tools -type f) extract_base.rb extract_zet.rb
	ruby extract_zet.rb

%.tp: %.rb ze_api.yaml zex_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb gen_probe_base.rb
	ruby $< > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_ze.so: ../utils/lttng/tracepoint_gen.h $(ZE_OBJS) $(ZE_INCL)
	echo nop

# objcopy --prefix-symbols= on ze_api.a content should allow a wrapper creation

clean:
	rm -f ze_api.yaml zex_api.yaml zet_api.yaml $(ZE_TP) $(ZE_OBJS) $(ZE_INCL) $(ZE_SRC)
	$(MAKE) -C ../utils clean
