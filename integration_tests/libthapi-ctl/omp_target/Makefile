CXXFLAGS = -g -Wall -Wextra $(WERROR) -I$(THAPI_INSTALL_DIR)/include
LDFLAGS = -L$(THAPI_INSTALL_DIR)/lib -lthapi-ctl -L$(LTTNG_INSTALL_DIR)/lib -llttng-ctl

vec_cuda: vec.cpp
	clang++ $(CXXFLAGS) $(LDFLAGS) -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda $^ -o $@

vec_intel: vec.cpp
	icpx $(CXXFLAGS) $(LDFLAGS) -qopenmp -fopenmp-targets=spir64 $^ -o $@

vec_hip: vec.cpp
	amdclang++ $(CXXFLAGS) $(LDFLAGS) -fopenmp --offload-arch=gfx1030 $^ -o $@

.PHONY: clean
clean:
	rm -f vec_cuda vec_intel vec_hip
