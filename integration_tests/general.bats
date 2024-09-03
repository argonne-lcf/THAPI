#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
}

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
