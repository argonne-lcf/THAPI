bats_require_minimum_version 1.5.0

# Sanity-check that the ZE tracer sees device-side activity when the user
# program reaches Level Zero via higher-level runtimes (OMP target offload
# and SYCL), not via direct ze_* calls.
#
# Skipped automatically when icpx is unavailable.

setup_file() {
  cd "$BATS_TEST_DIRNAME"
  if ! command -v icpx >/dev/null 2>&1; then
    skip "icpx not in PATH"
  fi
}

@test "build vec_add_omp" {
  icpx -O2 -fiopenmp -fopenmp-targets=spir64 vec_add_omp.cpp -o vec_add_omp
}

@test "vec_add_omp runs bare" {
  run -0 ./vec_add_omp
  [[ "$output" =~ "PASS" ]]
}

@test "vec_add_omp under iprof reports device-side activity" {
  iprof -j --analysis-output omp.json -- ./vec_add_omp
  device_total=$(jq '.device.data.Total.call // 0' omp.json)
  [ "$device_total" -ge 1 ]
}

@test "build vec_add_sycl" {
  icpx -O2 -fsycl vec_add_sycl.cpp -o vec_add_sycl
}

@test "vec_add_sycl runs bare" {
  run -0 ./vec_add_sycl
  [[ "$output" =~ "PASS" ]]
}

@test "vec_add_sycl under iprof reports device-side activity" {
  iprof -j --analysis-output sycl.json -- ./vec_add_sycl
  device_total=$(jq '.device.data.Total.call // 0' sycl.json)
  [ "$device_total" -ge 1 ]
}
