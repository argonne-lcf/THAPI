bats_require_minimum_version 1.5.0

# The sub-devices are created with num_devices_ret = NULL: a write on one of
# them must still be reported on its root device and sub-device (#572).
@test "backend_opencl_sub_devices_without_num_devices_ret" {
  gcc ./integration_tests/cl_sub_devices.c -I./backends/opencl/include -l:libOpenCL.so.1 -o cl_sub_devices
  run ./cl_sub_devices
  [ "$status" != 77 ] || skip "the OpenCL device cannot be partitioned"
  [ "$status" == 0 ]

  iprof --backends cl -e --analysis-output out_cl_sub_devices.txt -- ./cl_sub_devices
  grep "Subdevice pointer" out_cl_sub_devices.txt
}
