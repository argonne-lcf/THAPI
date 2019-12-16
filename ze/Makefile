all: tracer_ze.so

LTTNG_FLAGS=-fPIC -g -Wall -Werror -O3 -I./ -I../utils/

utils:
	$(MAKE) -C ../utils

ze_api.yaml: $(shell find include/core -type f) extract_base.rb extract_ze.rb
	ruby extract_ze.rb

zex_api.yaml: $(shell find include/core include/experimental -type f) extract_base.rb extract_zex.rb
	ruby extract_zex.rb

zet_api.yaml: $(shell find include/core include/experimental include/tools -type f) extract_base.rb extract_zet.rb
	ruby extract_zet.rb

tracer_ze.so: utils ze_api.yaml zex_api.yaml zet_api.yaml
	echo nop

# objcopy --prefix-symbols= on ze_api.a content should allow a wrapper creation

clean:
	rm -f ze_api.yaml zex_api.yaml zet_api.yaml
	$(MAKE) -C ../utils clean
