bats_require_minimum_version 1.5.0

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default_summary" {
   total_count=$( $IPROF --backend cl -- $THAPI_TEST_BIN | awk -F'|' '/Total/ {print int($4)}' )
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
   run -0 $IPROF -l -- $THAPI_TEST_BIN
   [[ "$output" =~ "THAPI: Perfetto trace location" ]]
   rm out.pftrace
}

@test "replay_summary" {
   $IPROF $THAPI_TEST_BIN
   $IPROF -r
}

@test "no-analysis_output" {
   run -0 $THAPI_TEST_BIN
   out1=$(echo "$output" | grep -v 'Max clock frequency')

   run -0 --separate-stderr $IPROF --no-analysis -- $THAPI_TEST_BIN
   out2=$(echo "$output" | grep -v 'Max clock frequency')
   err2=$stderr

   [[ "$out1" == "$out2" ]]
   [[ "$err2" =~ "THAPI: Trace location" ]]
}

@test "stderr_output" {
   run --separate-stderr $IPROF -- bash -c "echo \"error\" >&2"
   [[ "$stderr" =~ "error" ]]
}

@test "no-analysis_all" {
   $IPROF --no-analysis -- $THAPI_TEST_BIN
   $IPROF -r
   $IPROF -t -r | wc -l
   run -0 $IPROF -l -r
   [[ "$output" =~ "THAPI: Perfetto trace location" ]]
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
   run -0 $IPROF sleep 1
   [[ "$output" =~ "WARN -- : No source found" ]]
}

@test "read_stdin" {
   run bats_pipe echo "FOO" \| $IPROF cat
   [[ "$output" =~ "FOO" ]]
}
