all: tracer_ze.so

LTTNG_FLAGS=-fPIC -g -Wall -Werror -O3 -I./ -I./include/core -I./include/experimental -I./include/tools -I../utils/

utils:
	$(MAKE) -C ../utils

ze_api.yaml: $(shell find include/core -type f) extract_base.rb extract_ze.rb
	ruby extract_ze.rb

zex_api.yaml: $(shell find include/core include/experimental -type f) extract_base.rb extract_zex.rb
	ruby extract_zex.rb

zet_api.yaml: $(shell find include/core include/experimental include/tools -type f) extract_base.rb extract_zet.rb
	ruby extract_zet.rb

ze_tracepoints.tp: ze_api.yaml zex_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb gen_ze_probes.rb gen_probe_base.rb
	ruby gen_ze_probes.rb > ze_tracepoints.tp

ze_tracepoints.o: ze_tracepoints.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp ze_tracepoints.tp

zex_tracepoints.tp: ze_api.yaml zex_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb gen_zex_probes.rb gen_probe_base.rb
	ruby gen_zex_probes.rb > zex_tracepoints.tp

zex_tracepoints.o: zex_tracepoints.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp zex_tracepoints.tp

zet_tracepoints.tp: ze_api.yaml zex_api.yaml zet_api.yaml yaml_ast.rb ze_model.rb gen_zet_probes.rb gen_probe_base.rb
	ruby gen_zet_probes.rb > zet_tracepoints.tp

zet_tracepoints.o: zet_tracepoints.tp utils
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp zet_tracepoints.tp

tracer_ze.so: utils ze_tracepoints.o zex_tracepoints.o zet_tracepoints.o
	echo nop

# objcopy --prefix-symbols= on ze_api.a content should allow a wrapper creation

clean:
	rm -f ze_api.yaml zex_api.yaml zet_api.yaml ze_tracepoints.tp ze_tracepoints.c ze_tracepoints.h ze_tracepoints.o
	$(MAKE) -C ../utils clean
