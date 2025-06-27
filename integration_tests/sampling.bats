setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "sampling_heartbeat" {
   LTTNG_UST_SAMPLING_HEARTBEAT=1 $IPROF --no-analysis --sample --trace-output heartbeat_trace -- bash -c 'sleep 2'
   ./ici/bin/babeltrace_thapi  --no-restrict heartbeat_trace | grep heartbeat
   rm -rf heartbeat_trace
}

