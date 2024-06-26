#!/usr/bin/env bats

setup_file() {
   export IPROF=$THAPI_INSTALL_DIR/bin/iprof

   (cd integration_tests/libthapi-ctl;
    #ld --verbose | grep SEARCH_DIR | tr -s ' ;' \\012;
    #gcc -print-search-dirs | grep libraries | tr -s ' ;=:' \\012;
    make clean && make -j;
    #ldd hello-cl-startstop;
    #objdump -p hello-cl-startstop;
    )

   export THAPI_TEST_BIN=$(pwd)/integration_tests/libthapi-ctl/hello-cl-startstop
   export LD_LIBRARY_PATH=$THAPI_INSTALL_DIR/lib:$LD_LIBRARY_PATH

   # Note: the pocl version in Ubuntu 20.04 (old lts as of 2024) does not
   # support modern CPUs and will fail kernel compilation (notably does not work
   # on CI). To keep things simple, we do clEnqueueWriteBuffer only if pocl is the
   # default opencl platform.
   IS_POCL=$(clinfo -l | grep 'Platform #0' | grep -c "Portable Computing Language" || true)

   # echo "# IS_POCL=$IS_POCL" >&3

   TEST_ARGS=""
   if [ "$IS_POCL" = "0" ]; then
     TEST_ARGS="0 0 1"
   fi
   export TEST_ARGS IS_POCL
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "startstop-cl-trace-from-start" {
   $IPROF --debug 0 $THAPI_TEST_BIN $TEST_ARGS
   TRACE=$(ls -1t $THAPI_HOME/thapi-traces | head -n1)
   write_count=$($IPROF -r -j | jq '.device.data.clEnqueueWriteBuffer.call')
   # 0th call, plus 5 odd calls
   [ $write_count -eq 6 ]

   # echo "# IS_POCL=$IS_POCL" >&3

   if [ "$IS_POCL" = "0" ]; then
     kernel_count=$($IPROF -r -j | jq '.device.data.hello_world.call')
     [ $write_count -eq 6 ]
   fi
}

@test "startstop-cl-no-trace-from-start" {
   $IPROF --no-trace-from-start --debug 0 $THAPI_TEST_BIN $TEST_ARGS
   write_count=$($IPROF -r -j | jq '.device.data.clEnqueueWriteBuffer.call')
   # 5 odd calls, does not include 0 since profiling should be disabled at start
   [ $write_count -eq 5 ]

   # echo "# IS_POCL=$IS_POCL" >&3

   if [ "$IS_POCL" = "0" ]; then
     kernel_count=$($IPROF -r -j | jq '.device.data.hello_world.call')
     [ $write_count -eq 5 ]
     # echo "# IS_POCL=$IS_POCL write_count=$write_count" >&3
   fi
}
