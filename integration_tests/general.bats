bats_require_minimum_version 1.5.0

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default_summary" {
   $IPROF $THAPI_TEST_BIN
}

@test "default_trace" {
   $IPROF -t $THAPI_TEST_BIN | wc -l
}

@test "default_timeline" {
   $IPROF -l -- $THAPI_TEST_BIN
   rm out.pftrace
}

@test "archive_summary" {
   $IPROF --archive $THAPI_TEST_BIN
}

@test "replay_summary" {
   $IPROF $THAPI_TEST_BIN
   $IPROF -r
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

   $IPROF --trace-output trace_3 -l -- $THAPI_TEST_BIN
   $IPROF -l -r trace_3
   rm -rf trace_3 out.pftrace
}

@test "timeline_output" {
   $IPROF -l roger -- $THAPI_TEST_BIN
   rm roger
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
   run -3 $IPROF sleep 1
}

@test "read_stdin" {
   run -3 bats_pipe echo "FOO" \| $IPROF cat
   grep "FOO" "$output"
}
