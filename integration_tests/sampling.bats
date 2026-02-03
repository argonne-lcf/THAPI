@test "sampling_heartbeat" {
  LTTNG_UST_ZE_SAMPLING_ENERGY=0 LTTNG_UST_SAMPLING_HEARTBEAT=1 \
    iprof --no-analysis --sample --trace-output heartbeat_trace -- bash -c 'sleep 2'
  babeltrace_thapi --no-restrict heartbeat_trace | grep "{foo: 16}"
  [ $(babeltrace_thapi --no-restrict heartbeat_trace | grep -c "{foo: 32}") == 1 ]
}

@test "sampling_cxi" {
  # cleanup & prepare fake CXI device and telemetry
  rm -rf test_cxi test_device test_counter_list cxi_trace_test
  mkdir -p test_device/cxi0/device/telemetry/
  echo "0@12345.0" >test_device/cxi0/device/telemetry/test_counter
  echo "test_counter" >test_counter_list
  mkdir test_cxi
  ln -s "$(pwd)/test_device/cxi0" test_cxi/cxi0

  # after 1s, change the counter so a message is pushed
  (
    sleep 1
    echo "9999@12345.0" >test_device/cxi0/device/telemetry/test_counter
  ) &

  # run profiler with CXI backend for 2s
  LTTNG_UST_CXI_SAMPLING_CXI_COUNTERS_FILE="$(pwd)/test_counter_list" \
  LTTNG_UST_CXI_SAMPLING_CXI_BASE="$(pwd)/test_cxi" \
    iprof --no-analysis --sample --backend cxi \
    --trace-output cxi_trace_test -- bash -c 'sleep 2'

  # assert there is at least one CXI counter sample reporting the difference from the initial
  babeltrace_thapi --no-restrict cxi_trace_test | grep -a "value: 9999"

}
