bats_require_minimum_version 1.5.0

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default_summary" {
   total_count=$( $IPROF $THAPI_TEST_BIN | awk -F'|' '/Total/ {print int($4)}' )
   [ "$total_count" -ge 1 ]
}

@test "json_summary" {
   $IPROF --json --analysis-output out.json --json $THAPI_TEST_BIN
   total_count=$( $JQ '.host["1"].data.Total.call' out.json )
   [ "$total_count" -ge 1 ]
}

@test "archive_summary" {
   total_count=$( $IPROF --archive $THAPI_TEST_BIN | awk -F'|' '/Total/ {print int($4)}' )
   [ "$total_count" -ge 1 ]
}

@test "default_trace" {
   $IPROF -t $THAPI_TEST_BIN | wc -l
}

@test "default_timeline" {
   $IPROF -l -- $THAPI_TEST_BIN
   rm out.pftrace
}

@test "replay_summary" {
   $IPROF $THAPI_TEST_BIN
   $IPROF -r
}

@test "no-analysis_output" {
   run $THAPI_TEST_BIN
   out1=$(echo "$output" | grep -v 'Max clock frequency')

   run --separate-stderr $IPROF --no-analysis -- $THAPI_TEST_BIN
   out2=$(echo "$output" | grep -v 'Max clock frequency')
   err2=$stderr

   [[ "$out1" == "$out2" ]]
   [[ "$err2" =~ "THAPI: Trace location" ]]
}

@test "no-analysis_all" {
   $IPROF --no-analysis -- $THAPI_TEST_BIN
   $IPROF -r
   $IPROF -t -r | wc -l
   $IPROF -l -r
   rm out.pftrace
}

@test "trace-output_all" {
   $IPROF --trace-output trace_1 -- $THAPI_TEST_BIN
   $IPROF -r trace_1
   rm -rf trace_1

   $IPROF --trace-output trace_2 -t -- $THAPI_TEST_BIN | wc -l
   $IPROF -t -r trace_2 | wc -l
   rm -rf trace_2

   $IPROF --trace-output trace_3 -l out1.pftrace -- $THAPI_TEST_BIN
   $IPROF -l out2.pftrace -r trace_3
   rm -rf trace_3 out1.pftrace out2.pftrace
}

@test "timeline_output" {
   $IPROF -l roger.pftrace -- $THAPI_TEST_BIN
   rm roger.pftrace
}

# Assert Failure
@test "replay_negative" {
   $IPROF  -- $THAPI_TEST_BIN
   run $IPROF -t -r
   [ "$status" != 0 ]
   run $IPROF -l -r
   [ "$status" != 0 ]

   $IPROF -l -- $THAPI_TEST_BIN
   run $IPROF -t -r
   [ "$status" != 0 ]
   rm out.pftrace
}

@test "error_code_when_no_trace" {
   run $IPROF sleep 1
   [[ "$output" =~ "WARN -- : No source found" ]]
}

@test "read_stdin" {
   run bats_pipe echo "FOO" \| $IPROF cat
   [[ "$output" =~ "FOO" ]]
}

@test "thapi_start_stop" {
  cc -I${THAPI_INC_DIR} ./integration_tests/thapi_start_stop.c -o thapi_start_stop \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi
  $IPROF --trace-output trace_toggle --no-analysis -- ./thapi_start_stop

  start_count=`babeltrace2 trace_toggle | grep lttng_ust_toggle:start | wc -l`
  [ "$start_count" -eq 1 ]

  stop_count=`babeltrace2 trace_toggle | grep lttng_ust_toggle:stop | wc -l`
  [ "$stop_count" -eq 2 ]
}
