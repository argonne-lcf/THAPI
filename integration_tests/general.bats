bats_require_minimum_version 1.5.0

@test "pkg-config_file" {
  pkg-config --modversion thapi
}

@test "default_summary" {
  total_count=$(iprof --backend cl -- clinfo | awk -F'|' '/Total/ {print int($4)}')
  [ "$total_count" -ge 1 ]
}

@test "json_summary" {
  iprof --json --analysis-output out.json --json clinfo
  total_count=$(jq '.host["1"].data.Total.call' out.json)
  [ "$total_count" -ge 1 ]
}

@test "archive_summary" {
  total_count=$(iprof --backend cl --archive clinfo | awk -F'|' '/Total/ {print int($4)}')
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
  iprof -- clinfo
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

@test "toggle_api" {
  rm -rf toggle_traces 2> /dev/null

  cc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle.c -o thapi_toggle \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi
  $IPROF --trace-output toggle_traces --no-analysis -- ./thapi_toggle

  start_count=`babeltrace2 toggle_traces | grep lttng_ust_toggle:start | wc -l`
  [ "$start_count" -eq 1 ]

  stop_count=`babeltrace2 toggle_traces | grep lttng_ust_toggle:stop | wc -l`
  [ "$stop_count" -eq 2 ]
}

toggle_count_base() {
  rm -rf toggle_traces 2> /dev/null

  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$3 timeout 40s $MPIRUN -n $1 $IPROF --trace-output toggle_traces --no-analysis -- ./thapi_toggle_mpi $2

  trace_metadata_file=`find toggle_traces -iname metadata`
  trace_metadata_dir=$(dirname "${trace_metadata_file}")
  traces=$(babeltrace2 --plugin-path=${THAPI_LIB_DIR} \
    --component source:source.ctf.fs --params "inputs=[\"${trace_metadata_dir}\"]" \
    --component=filter:filter.metababel_filter.btx \
    --component=sink:sink.text.pretty)

  echo $traces
}

toggle_count_traces() {
  traces=$(toggle_count_base $1 $2 $3)
  echo $traces | sed -e "s/ \[/@[/g" | sed "s/@/\n/g" | grep . | wc -l
}

@test "toggle_plugin_mpi_np_1" {
  mpicc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle_mpi.c -o thapi_toggle_mpi \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi

  count_0=$(toggle_count_traces 1 0 100)
  count_1=$(toggle_count_traces 1 1 101)
  count_2=$(toggle_count_traces 1 2 102)

  [ "$count_2" -eq 0 ]
  [ "$count_0" -gt "$count_1" ]
}

toggle_count_vpids() {
  traces=$(toggle_count_base $1 $2 $3)
  echo $traces | sed -e "s/ - /, /g" | sed -e "s/,/\n/g" | grep vpid | sort | uniq | wc -l
}

@test "toggle_plugin_mpi_np_2" {
  mpicc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle_mpi.c -o thapi_toggle_mpi \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi

  count_0=$(toggle_count_vpids 2 0 200)
  count_1=$(toggle_count_vpids 2 1 201)
  count_2=$(toggle_count_vpids 2 2 202)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}
