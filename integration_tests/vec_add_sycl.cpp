/* SYCL vector add — sanity reproducer for the ZE tracer via the SYCL
 * runtime layer.
 * Build: icpx -fsycl vec_add_sycl.cpp -o vec_add_sycl */
#include <cstdio>
#include <sycl/sycl.hpp>
#include <vector>

int main() {
  constexpr int N = 1024;
  std::vector<float> a(N, 1.0f), b(N, 2.0f), c(N, 0.0f);

  {
    sycl::queue q;
    sycl::buffer<float, 1> ba(a.data(), N);
    sycl::buffer<float, 1> bb(b.data(), N);
    sycl::buffer<float, 1> bc(c.data(), N);
    q.submit([&](sycl::handler &h) {
      auto aa = ba.get_access<sycl::access::mode::read>(h);
      auto ab = bb.get_access<sycl::access::mode::read>(h);
      auto ac = bc.get_access<sycl::access::mode::write>(h);
      h.parallel_for(sycl::range<1>(N), [=](sycl::id<1> i) { ac[i] = aa[i] + ab[i]; });
    });
    q.wait();
  }

  for (int i = 0; i < N; ++i) {
    if (c[i] != 3.0f) {
      fprintf(stderr, "FAIL: c[%d]=%f expected=3.0\n", i, c[i]);
      return 1;
    }
  }
  printf("PASS: vec_add_sycl\n");
  return 0;
}
