#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=${THAPI_HOME:-${PWD}}
   export THAPI_INSTALL_DIR=${THAPI_INSTALL_DIR:-${PWD}/build/ici/}
   export THAPI_BIN_DIR=${THAPI_BIN_DIR:-${THAPI_INSTALL_DIR}/bin}
   export THAPI_INC_DIR=${THAPI_INC_DIR:-${THAPI_INSTALL_DIR}/include}
   export THAPI_LIB_DIR=${THAPI_LIB_DIR:-${THAPI_INSTALL_DIR}/lib}

   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
   export BBT=${THAPI_BIN_DIR}/babeltrace_thapi
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

get_unique_jobid() {
  echo ${BATS_TEST_NAME}.${RANDOM}
}

@test "toggle_api" {
  rm -rf toggle_traces 2> /dev/null

  cc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle.c -o thapi_toggle \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi

  $IPROF --trace-output toggle_traces --no-analysis -- ./thapi_toggle
  dir=$(ls -d -1 ./toggle_traces/*/)

  start_count=`$BBT -c $dir | grep lttng_ust_toggle:start | wc -l`
  [ "$start_count" -eq 1 ]

  stop_count=`$BBT -c $dir | grep lttng_ust_toggle:stop | wc -l`
  [ "$stop_count" -eq 2 ]
}

toggle_count_base() {
  rm -rf toggle_traces 2> /dev/null

  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n $1 $IPROF --trace-output toggle_traces --no-analysis -- ./thapi_toggle_mpi $2

  trace_metadata_file=`find toggle_traces -iname metadata`
  trace_metadata_dir=$(dirname "${trace_metadata_file}")
  traces=$(babeltrace2 --plugin-path=${THAPI_LIB_DIR} \
    --component source:source.ctf.fs --params "inputs=[\"${trace_metadata_dir}\"]" \
    --component=filter:filter.metababel_filter.btx \
    --component=sink:sink.text.pretty)

  echo $traces
}

toggle_count_traces() {
  traces=$(toggle_count_base $1 $2)
  echo $traces | sed -e "s/ \[/\n[/g" | grep . | wc -l
}

@test "toggle_plugin_mpi_np_1" {
  mpicc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle_mpi.c -o thapi_toggle_mpi \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi

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
  mpicc -I${THAPI_INC_DIR} ./integration_tests/thapi_toggle_mpi.c -o thapi_toggle_mpi \
    -Wl,-rpath,${THAPI_LIB_DIR} -L${THAPI_LIB_DIR} -lThapi

  count_0=$(toggle_count_vpids 2 0)
  count_1=$(toggle_count_vpids 2 1)
  count_2=$(toggle_count_vpids 2 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}
