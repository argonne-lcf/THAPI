#!/usr/bin/env bats

setup_file() {
  export THAPI_INCFLAGS="-I$(pkg-config --variable=includedir thapi)"
  export THAPI_LDFLAGS="-Wl,-rpath,$(pkg-config --variable=libdir thapi) $(pkg-config --libs thapi)"
}

teardown_file() {
  rm -rf $THAPI_HOME/thapi-traces
}

get_unique_jobid() {
  echo ${BATS_TEST_NAME}.${RANDOM}
}

@test "toggle_api" {
  rm -rf toggle_traces 2>/dev/null

  cc ${THAPI_INCFLAGS} ./integration_tests/toggle.c -o toggle ${THAPI_LDFLAGS}

  iprof --trace-output toggle_traces --no-analysis -- ./toggle
  dir=$(ls -d -1 ./toggle_traces/*/)

  start_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:start | wc -l)
  [ "$start_count" -eq 1 ]

  stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:stop | wc -l)
  [ "$stop_count" -eq 2 ]
}

toggle_count_base() {
  rm -rf toggle_traces 2>/dev/null

  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n $1 \
    iprof --trace-output toggle_traces --no-analysis -- ./toggle_mpi $2

  traces=$(babeltrace_thapi ./toggle_traces)

  echo $traces
}

toggle_count_traces() {
  traces=$(toggle_count_base $1 $2)
  echo $traces | sed -e "s/ \[/\n[/g" | grep . | wc -l
}

@test "toggle_plugin_mpi_np_1" {
  mpicc ${THAPI_INCFLAGS} ./integration_tests/toggle_mpi.c -o toggle_mpi ${THAPI_LDFLAGS}

  count_0=$(toggle_count_traces 1 0)
  count_1=$(toggle_count_traces 1 1)
  count_2=$(toggle_count_traces 1 2)

  [ "$count_2" -eq 0 ]
  [ "$count_0" -gt "$count_1" ]
}

toggle_count_vpids() {
  traces=$(toggle_count_base $1 $2)
  echo $traces | sed -e "s/ - /, /g" | sed -e "s/,/\n/g" | grep vpid | sort | uniq | wc -l
}

@test "toggle_plugin_mpi_np_2" {
  mpicc ${THAPI_INCFLAGS} ./integration_tests/toggle_mpi.c -o toggle_mpi ${THAPI_LDFLAGS}

  count_0=$(toggle_count_vpids 2 0)
  count_1=$(toggle_count_vpids 2 1)
  count_2=$(toggle_count_vpids 2 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}
