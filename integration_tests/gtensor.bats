#!/usr/bin/env bats

setup_file() {
   export IPROF=$THAPI_INSTALL_DIR/bin/iprof

   bdir=build-$GTENSOR_DEVICE

   (cd integration_tests/gtensor;
    cmake -S . -B $bdir -DGTENSOR_DEVICE=$GTENSOR_DEVICE -DTHAPI_PATH=$THAPI_INSTALL_DIR \
    && cmake --build $bdir)

   export THAPI_TEST_BIN=$(pwd)/integration_tests/gtensor/$bdir/axpy_start_stop
   export LD_LIBRARY_PATH=$THAPI_INSTALL_DIR/lib:$LD_LIBRARY_PATH
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "axpy-${GTENSOR_DEVICE}-trace-from-start" {
   $IPROF --debug 0 $THAPI_TEST_BIN $TEST_ARGS
   # NOTE: gtensor kernel names are complex, but all start with gt::, look it up rather
   # than hard coding and making it fragile. Includes quotes.
   kernel_name=$($IPROF -r -j | jq '.device.data | keys[]' | grep 'gt::')
   write_count=$($IPROF -r -j | jq ".device.data | .[${kernel_name}].call")
   # 0th call, plus 5 odd calls
   [ $write_count -eq 6 ]
}

@test "axpy-${GTENSOR_DEVICE}-no-trace-from-start" {
   $IPROF --no-trace-from-start --debug 0 $THAPI_TEST_BIN $TEST_ARGS
   kernel_name=$($IPROF -r -j | jq '.device.data | keys[]' | grep 'gt::')
   write_count=$($IPROF -r -j | jq ".device.data | .[${kernel_name}].call")
   # 5 odd calls, does not include 0 since profiling should be disabled at start
   [ $write_count -eq 5 ]
}
