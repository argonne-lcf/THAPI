#!/usr/bin/env bats

setup_file() {
   export IPROF=$THAPI_INSTALL_DIR/bin/iprof

   (cd integration_tests/libthapi-ctl;
    ld --verbose | grep SEARCH_DIR | tr -s ' ;' \\012;
    gcc -print-search-dirs | grep libraries | tr -s ' ;=:' \\012;
    make -j;
    ldd hello-cl-startstop;
    objdump -p hello-cl-startstop)

   export THAPI_TEST_BIN=$(pwd)/integration_tests/libthapi-ctl/hello-cl-startstop
   export LD_LIBRARY_PATH=$THAPI_INSTALL_DIR/lib:$LD_LIBRARY_PATH
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "startstop-cl-profile-from-start" {
   $IPROF --debug 0 $THAPI_TEST_BIN
   TRACE=$(ls -1t $THAPI_HOME/thapi-traces | head -n1)
   write_count=$($IPROF -r -j | jq '.device.data.clEnqueueWriteBuffer.call')
   # 0th call, plus 5 odd calls
   [ $write_count -eq 6 ]
}

@test "startstop-cl-no-profile-from-start" {
   $IPROF --no-profile-from-start --debug 0 $THAPI_TEST_BIN
   write_count=$($IPROF -r -j | jq '.device.data.clEnqueueWriteBuffer.call')
   # 5 odd calls, does not include 0 since profiling should be disabled at start
   [ $write_count -eq 5 ]
}
