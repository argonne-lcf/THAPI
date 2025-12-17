bats_require_minimum_version 1.5.0

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default_summary" {
   total_count=$( iprof --backend cl -- clinfo | awk -F'|' '/Total/ {print int($4)}' )
   [ "$total_count" -ge 1 ]
}

@test "json_summary" {
   iprof --json --analysis-output out.json --json clinfo
   total_count=$( jq '.host["1"].data.Total.call' out.json )
   [ "$total_count" -ge 1 ]
}

@test "archive_summary" {
   total_count=$( iprof --backend cl --archive clinfo | awk -F'|' '/Total/ {print int($4)}' )
   [ "$total_count" -ge 1 ]
}

@test "default_trace" {
   iprof -t clinfo | wc -l
}

@test "default_timeline" {
   run -0 iprof -l -- clinfo
   [[ "$output" =~ "THAPI: Perfetto trace location" ]]
   rm out.pftrace
}

@test "replay_summary" {
   iprof clinfo
   iprof -r
}

@test "no-analysis_output" {
   run -0 clinfo
   out1=$(echo "$output" | grep -v 'Max clock frequency' | grep -v '  Device LUID')

   run -0 --separate-stderr iprof --no-analysis -- clinfo
   out2=$(echo "$output" | grep -v 'Max clock frequency' | grep -v '  Device LUID')
   err2=$stderr

   [[ "$out1" == "$out2" ]]
   [[ "$err2" =~ "THAPI: Trace location" ]]
}

@test "stderr_output" {
   run --separate-stderr iprof -- bash -c "echo \"error\" >&2"
   [[ "$stderr" =~ "error" ]]
}

@test "no-analysis_all" {
   iprof --no-analysis -- clinfo
   iprof -r
   iprof -t -r | wc -l
   run -0 iprof -l -r
   [[ "$output" =~ "THAPI: Perfetto trace location" ]]
   rm out.pftrace
}

@test "trace-output_all" {
   iprof --trace-output trace_1 -- clinfo
   iprof -r trace_1
   rm -rf trace_1

   iprof --trace-output trace_2 -t -- clinfo | wc -l
   iprof -t -r trace_2 | wc -l
   rm -rf trace_2

   iprof --trace-output trace_3 -l out1.pftrace -- clinfo
   iprof -l out2.pftrace -r trace_3
   rm -rf trace_3 out1.pftrace out2.pftrace
}

@test "timeline_output" {
   iprof -l roger.pftrace -- clinfo
   rm roger.pftrace
}

# Assert Failure
@test "replay_negative" {
   iprof  -- clinfo
   run iprof -t -r
   [ "$status" != 0 ]
   run iprof -l -r
   [ "$status" != 0 ]

   iprof -l -- clinfo
   run iprof -t -r
   [ "$status" != 0 ]
   rm out.pftrace
}

@test "error_code_when_no_trace" {
   run -0 iprof sleep 1
   [[ "$output" =~ "WARN -- : No source found" ]]
}

@test "read_stdin" {
   run bats_pipe echo "FOO" \| iprof cat
   [[ "$output" =~ "FOO" ]]
}
