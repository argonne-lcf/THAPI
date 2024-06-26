CXXFLAGS = -Wall -Wextra $(WERROR)

vec_cuda: vec.cpp
	clang -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda $^ -o $@

vec_intel: vec.cpp
	icpx -qopenmp -fopenmp-targets=spir64 $^ -o $@

.PHONY: clean
clean:
	rm -f vec_cuda vec_intel
