.DELETE_ON_ERROR:
.SUFFIXES:

CUDA_PROBES = cuda_tracepoints cudart_tracepoints
CUDA_TP = $(CUDA_PROBES:=.tp)
CUDA_OBJS = $(CUDA_PROBES:=.o)
CUDA_INCL = $(CUDA_PROBES:=.h)
CUDA_SRC = $(CUDA_PROBES:=.c)

CUDA_STATIC_PROBES = cuda_profiling
CUDA_STATIC_TP = $(CUDA_STATIC_PROBES:=.tp)
CUDA_STATIC_OBJS = $(CUDA_STATIC_PROBES:=.o)
CUDA_STATIC_INCL = $(CUDA_STATIC_PROBES:=.h)
CUDA_STATIC_SRC = $(CUDA_STATIC_PROBES:=.c)

CUDA_EXTRACT = extract_base.rb modified_include
CUDA_EXTRACTED = cuda_api.yaml cudart_api.yaml
CUDA_MODEL = cuda_model.rb yaml_ast.rb cuda_meta_parameters.yaml cudart_meta_parameters.yaml $(CUDA_EXTRACTED)

CUDA_BINDINGS = cuda_bindings.rb cuda_refinements.rb

.SECONDARY: $(CUDA_TP)

all: tracer_cuda.so cuda_library.rb babeltrace_cuda_lib.rb $(CUDA_BINDINGS)

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-nonnull -Werror -O3 -I./ -I./include/ -I../utils/

modified_include: include headers.patch
	cp -r include/ modified_include/
	patch -i ../headers.patch -d modified_include/ -s -p1

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

cuda_api.yaml: modified_include $(CUDA_EXTRACT) extract_cuda.rb
	ruby extract_cuda.rb

cudart_api.yaml: modified_include $(CUDA_EXTRACT) extract_cudart.rb
	ruby extract_cudart.rb

cuda_profiling.tp: gen_cuda_custom_probes.rb ../utils/LTTng.rb $(CUDA_MODEL)
	ruby gen_cuda_custom_probes.rb lttng_ust_cuda_profiling > $@

%.tp: %.rb $(CUDA_MODEL) gen_probe_base.rb
	ruby $*.rb > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_cuda.c: gen_cuda.rb tracer_cuda_helpers.include.c $(CUDA_MODEL)
	ruby $< > $@

tracer_cuda.so: ../utils/lttng/tracepoint_gen.h tracer_cuda.c $(CUDA_OBJS) $(CUDA_INCL) $(CUDA_STATIC_OBJS) $(CUDA_STATIC_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./include/core -I./include/tools -I./ -I../utils/ -o tracer_cuda.so -shared -fPIC tracer_cuda.c $(CUDA_OBJS) $(CUDA_STATIC_OBJS) -llttng-ust -ldl -lffi

cuda_library.rb: gen_cuda_library.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

babeltrace_cuda_lib.rb: gen_babeltrace_cuda_lib.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

cuda_refinements.rb: gen_cuda_refinements.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

cuda_bindings.rb: gen_cuda_bindings.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

# objcopy --prefix-symbols= on cuda_api.a content should allow a wrapper creation

clean:
	rm -f \
		cuda_api.yaml cudart_api.yaml\
		$(CUDA_TP) $(CUDA_OBJS) $(CUDA_INCL) $(CUDA_SRC)\
		$(CUDA_STATIC_TP) $(CUDA_STATIC_OBJS) $(CUDA_STATIC_INCL) $(CUDA_STATIC_SRC)\
		$(CUDA_BINDINGS)\
		tracer_cuda.c tracer_cuda.so cuda_library.rb babeltrace_cuda_lib.rb
	rm -fr modified_include
	$(MAKE) -C ../utils clean
