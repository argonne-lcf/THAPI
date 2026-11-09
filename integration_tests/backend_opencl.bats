bats_require_minimum_version 1.5.0

setup_file() {
  gcc ./integration_tests/cl_sub_devices.c -I./backends/opencl/include -l:libOpenCL.so.1 -o cl_sub_devices
}

teardown_file() {
  rm -f cl_sub_devices
}

# The sub-devices are created with num_devices_ret = NULL: a write on one of
# them must still be reported on its root device and sub-device (#572).
@test "backend_opencl_sub_devices_without_num_devices_ret" {
  run ./cl_sub_devices
  [ "${status}" -ne 77 ] || skip "the OpenCL device cannot be partitioned"
  [ "${status}" -eq 0 ]

  run -0 iprof --backends cl -e -- ./cl_sub_devices
  grep -q '^Device profiling .*| Device pointer: [0-9]* | Subdevice pointer: [0-9]* |' <<<"${output}"
}
