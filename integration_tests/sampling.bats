#!/usr/bin/env bats

load test_env_vars.bash

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

