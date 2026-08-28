#!/usr/bin/env bats

setup_file() {
  export THAPI_INCFLAGS="-I$(pkg-config --variable=includedir thapi)"
  export THAPI_LDFLAGS="-Wl,-rpath,$(pkg-config --variable=libdir thapi) $(pkg-config --libs thapi)"
  # needed for toggle api dlopen test.
  export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(pkg-config --variable=libdir thapi)
}

get_unique_jobid() {
  echo ${BATS_TEST_NAME}.${RANDOM}
}

@test "toggle_api" {
  rm -rf toggle_traces 2>/dev/null

  cc ${THAPI_INCFLAGS} ./integration_tests/toggle.c -o toggle ${THAPI_LDFLAGS}
  iprof --trace-output toggle_traces --no-analysis -- ./toggle

  # Make sure auto_stop comes before stop.
  babeltrace_thapi ./toggle_traces | awk 'BEGIN { seen_auto = 0 }
  $0 ~ /lttng_ust_toggle:auto_stop/ { seen_auto = 1 }
  $0 ~ /lttng_ust_toggle:stop/ { if (seen_auto == 1) { exit 0 } else { exit 1 } }
  '

  # Check expected trace counts.
  dir=$(ls -d -1 ./toggle_traces/*/)
  start_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:start | wc -l)
  [ "$start_count" -eq 1 ]

  stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:stop | wc -l)
  [ "$stop_count" -eq 1 ]

  auto_stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:auto_stop | wc -l)
  [ "$auto_stop_count" -eq 1 ]
}

@test "toggle_api_dlopen" {
  rm -rf toggle_traces 2>/dev/null

  cc ./integration_tests/toggle_dlopen.c -o toggle_dlopen -ldl
  iprof --trace-output toggle_traces --no-analysis -- ./toggle_dlopen

  # Check expected trace counts.
  dir=$(ls -d -1 ./toggle_traces/*/)
  start_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:start | wc -l)
  [ "$start_count" -eq 2 ]

  stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:stop | wc -l)
  [ "$stop_count" -eq 2 ]

  auto_stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:auto_stop | wc -l)
  [ "$auto_stop_count" -eq 2 ]
}

# Trace and analyse in a single iprof call: `--toggle-on` gates the events on the
# lttng_ust_toggle start/stop tracepoints, and iprof prints the tally to stdout.
count_base() {
  rm -rf toggle_traces 2>/dev/null

  mpicc ${THAPI_INCFLAGS} ./integration_tests/toggle_mpi.c -o toggle_mpi ${THAPI_LDFLAGS}

  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s mpirun -n $1 \
    iprof --toggle-on --trace-output toggle_traces -- ./toggle_mpi $2
}

# Count the number of traced API calls: the tally's `Total` row, whose 4th
# `|`-separated column is the call count (see general.bats `default_summary`).
count_traces() {
  count_base $1 $2 | awk -F'|' '/Total/ {c = int($4)} END {print c + 0}'
}

@test "toggle_plugin_mpi_np_1" {
  count_0=$(count_traces 1 0)
  count_1=$(count_traces 1 1)
  count_2=$(count_traces 1 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}

# Count the number of processes that emitted traced API calls: the `Processes`
# field of the tally header.
count_processors() {
  count_base $1 $2 | grep -oP '\d+(?= Processes)' || echo 0
}

@test "toggle_plugin_mpi_np_2" {
  count_0=$(count_processors 2 0)
  count_1=$(count_processors 2 1)
  count_2=$(count_processors 2 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}
