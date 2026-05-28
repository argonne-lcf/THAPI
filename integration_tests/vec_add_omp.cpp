/* OMP target offload vector add — sanity reproducer for the ZE tracer
 * via the OMP runtime layer.
 * Build: icpx -fiopenmp -fopenmp-targets=spir64 vec_add_omp.cpp -o vec_add_omp */
#include <cstdio>
#include <vector>

int main() {
  constexpr int N = 1024;
  std::vector<float> a(N, 1.0f), b(N, 2.0f), c(N, 0.0f);

  float *pa = a.data(), *pb = b.data(), *pc = c.data();
  #pragma omp target teams distribute parallel for map(to:pa[0:N], pb[0:N]) map(from:pc[0:N])
  for (int i = 0; i < N; ++i)
    pc[i] = pa[i] + pb[i];

  for (int i = 0; i < N; ++i) {
    if (c[i] != 3.0f) {
      fprintf(stderr, "FAIL: c[%d]=%f expected=3.0\n", i, c[i]);
      return 1;
    }
  }
  printf("PASS: vec_add_omp\n");
  return 0;
}
