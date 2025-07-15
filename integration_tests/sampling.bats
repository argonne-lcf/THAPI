#!/usr/bin/env bats

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "sampling_heartbeat" {
   LTTNG_UST_ZE_SAMPLING_ENERGY=0 LTTNG_UST_SAMPLING_HEARTBEAT=1 \
       $THAPI_BIN_DIR/iprof --no-analysis --sample --trace-output heartbeat_trace --\
       bash -c 'sleep 2'
   $THAPI_BIN_DIR/babeltrace_thapi --no-restrict heartbeat_trace | grep  "{foo: 16}"
   [ $("$THAPI_BIN_DIR"/babeltrace_thapi --no-restrict heartbeat_trace | grep -c "{foo: 32}")  == 1 ]
}

@test "sampling_cxi" {
    # cleanup & prepare fake CXI device and telemetry
    rm -rf test_cxi test_device test_counter_list cxi_trace_test
    mkdir -p test_device/cxi0/device/telemetry/
    echo "9999@12345.0" > test_device/cxi0/device/telemetry/test_counter
    echo "test_counter" > test_counter_list
    mkdir test_cxi
    ln -s "$(pwd)/test_device/cxi0" test_cxi/cxi0

    # run profiler with CXI backend
    LTTNG_UST_CXI_SAMPLING_CXI_COUNTERS_FILE="$(pwd)/test_counter_list" \
    LTTNG_UST_CXI_SAMPLING_CXI_BASE="$(pwd)/test_cxi" \
    $THAPI_BIN_DIR/iprof --no-analysis --sample --backend cxi \
        --trace-output cxi_trace_test -- bash -c 'sleep 2'

    # assert there's at least one CXI counter sample in the trace
    $THAPI_BIN_DIR/babeltrace_thapi --no-restrict cxi_trace_test \
        | grep "{ifname: cxi0, counter: test_counter, value: 9999}"
}

