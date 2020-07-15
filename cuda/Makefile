.DELETE_ON_ERROR:
.SUFFIXES:

CUDA_PROBES = cuda_tracepoints
CUDA_TP = $(CUDA_PROBES:=.tp)
CUDA_OBJS = $(CUDA_PROBES:=.o)
CUDA_INCL = $(CUDA_PROBES:=.h)
CUDA_SRC = $(CUDA_PROBES:=.c)

CUDA_STATIC_PROBES = 
CUDA_STATIC_TP = $(CUDA_STATIC_PROBES:=.tp)
CUDA_STATIC_OBJS = $(CUDA_STATIC_PROBES:=.o)
CUDA_STATIC_INCL = $(CUDA_STATIC_PROBES:=.h)
CUDA_STATIC_SRC = $(CUDA_STATIC_PROBES:=.c)

CUDART_PROBES = cudart_tracepoints
CUDART_TP = $(CUDART_PROBES:=.tp)
CUDART_OBJS = $(CUDART_PROBES:=.o)
CUDART_INCL = $(CUDART_PROBES:=.h)
CUDART_SRC = $(CUDART_PROBES:=.c)

CUDART_STATIC_PROBES = 
CUDART_STATIC_TP = $(CUDART_STATIC_PROBES:=.tp)
CUDART_STATIC_OBJS = $(CUDART_STATIC_PROBES:=.o)
CUDART_STATIC_INCL = $(CUDART_STATIC_PROBES:=.h)
CUDART_STATIC_SRC = $(CUDART_STATIC_PROBES:=.c)

CUDA_EXTRACT = extract_base.rb modified_include
CUDA_EXTRACTED = cuda_api.yaml
CUDART_EXTRACTED = cudart_api.yaml
CUDA_MODEL = cuda_model.rb ../utils/yaml_ast.rb cuda_meta_parameters.yaml $(CUDA_EXTRACTED)
CUDART_MODEL = cudart_model.rb ../utils/yaml_ast.rb cudart_meta_parameters.yaml $(CUDART_EXTRACTED)

.SECONDARY: $(CUDA_TP)

all: tracer_cuda.so tracer_cudart.so cuda_library.rb babeltrace_cuda_lib.rb

LTTNG_FLAGS=-fPIC -g -Wall -Wextra -Wno-unused-parameter -Wno-type-limits -Wno-nonnull -Werror -O3 -I./ -I./include/ -I../utils/

modified_include: include/cuda.h include/cuda_runtime_api.h headers.patch
	rm -fr modified_include
	cp -r include/ modified_include/
	patch -i ../headers.patch -d modified_include/ -s -p1

../utils/lttng/tracepoint_gen.h: ../utils/tracepoint_gen.rb
	ruby ../utils/tracepoint_gen.rb 25 > ../utils/lttng/tracepoint_gen.h

cuda_api.yaml: modified_include/cuda.h $(CUDA_EXTRACT) extract_cuda.rb
	ruby extract_cuda.rb

cudart_api.yaml: modified_include/cuda_runtime_api.h $(CUDA_EXTRACT) extract_cudart.rb
	ruby extract_cudart.rb

cuda_profiling.tp: gen_cuda_custom_probes.rb ../utils/LTTng.rb $(CUDA_MODEL)
	ruby gen_cuda_custom_probes.rb lttng_ust_cuda_profiling > $@

cuda_tracepoints.tp: cuda_tracepoints.rb $(CUDA_MODEL) gen_probe_base.rb
	ruby cuda_tracepoints.rb > $@

cudart_tracepoints.tp: cudart_tracepoints.rb $(CUDART_MODEL) gen_probe_base.rb
	ruby cudart_tracepoints.rb > $@

%.c %.h %.o: %.tp ../utils/lttng/tracepoint_gen.h
	CFLAGS="$(LTTNG_FLAGS)" lttng-gen-tp $<

tracer_cuda.c: gen_cuda.rb tracer_cuda_helpers.include.c $(CUDA_MODEL)
	ruby $< > $@

tracer_cudart.c: gen_cudart.rb tracer_cudart_helpers.include.c $(CUDART_MODEL)
	ruby $< > $@

tracer_cuda.so: ../utils/lttng/tracepoint_gen.h tracer_cuda.c $(CUDA_OBJS) $(CUDA_INCL) $(CUDA_STATIC_OBJS) $(CUDA_STATIC_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./include/core -I./include/tools -I./ -I../utils/ -o tracer_cuda.so -shared -fPIC tracer_cuda.c $(CUDA_OBJS) $(CUDA_STATIC_OBJS) -llttng-ust -ldl -lffi

tracer_cudart.so: ../utils/lttng/tracepoint_gen.h tracer_cudart.c $(CUDART_OBJS) $(CUDART_INCL) $(CUDART_STATIC_OBJS) $(CUDART_STATIC_INCL)
	gcc -g -O3 -Wall -Wextra -Werror -I./include/core -I./include/tools -I./ -I../utils/ -o tracer_cudart.so -shared -fPIC tracer_cudart.c $(CUDART_OBJS) $(CUDART_STATIC_OBJS) -llttng-ust -ldl -lffi

cuda_library.rb: gen_cuda_library.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

babeltrace_cuda_lib.rb: gen_babeltrace_cuda_lib.rb gen_cuda_library_base.rb gen_probe_base.rb $(CUDA_MODEL)
	ruby $< > $@

# objcopy --prefix-symbols= on cuda_api.a content should allow a wrapper creation

clean:
	rm -f \
		cuda_api.yaml cudart_api.yaml\
		$(CUDA_TP) $(CUDA_OBJS) $(CUDA_INCL) $(CUDA_SRC)\
		$(CUDART_TP) $(CUDART_OBJS) $(CUDART_INCL) $(CUDART_SRC)\
		$(CUDA_STATIC_TP) $(CUDA_STATIC_OBJS) $(CUDA_STATIC_INCL) $(CUDA_STATIC_SRC)\
		$(CUDART_STATIC_TP) $(CUDART_STATIC_OBJS) $(CUDART_STATIC_INCL) $(CUDART_STATIC_SRC)\
		tracer_cuda.c tracer_cuda.so tracer_cudart.c tracer_cudart.so cuda_library.rb babeltrace_cuda_lib.rb
	rm -fr modified_include
	$(MAKE) -C ../utils clean
