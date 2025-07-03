#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export BBT=$THAPI_BIN_DIR/babeltrace_thapi
   export MPIRUN=${MPIRUN:-mpirun}
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

get_unique_jobid() {
  echo ${BATS_TEST_NAME}.${RANDOM}
}

RUN_FS_DAEMON() {
  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN $@
}

RUN_MPI_DAEMON() {
  THAPI_SYNC_DAEMON=mpi THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN $@
}

@test "sync_daemon_fs" {
   RUN_FS_DAEMON -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_fs" {
   RUN_FS_DAEMON -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

calc_iprof_vpids() {
  dir=$(ls -d -1 ./$1_traces/*/)
  $BBT -c $dir | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l
}

exec_iprof_vpids() {
  rm -rf $1_traces
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld

  if [ "$1" == "mpi" ]; then
    RUN_MPI_DAEMON -n 2 $IPROF --no-analysis --trace-output $1_traces -- ./mpi_helloworld > /dev/null
  elif [ "$1" == "fs" ]; then
    RUN_FS_DAEMON -n 2 $IPROF --no-analysis --trace-output $1_traces -- ./mpi_helloworld > /dev/null
  fi
}

@test "iprof_vpids_mpi" {
  exec_iprof_vpids mpi
  [ "$(calc_iprof_vpids mpi)" -eq 2 ]
}

@test "iprof_vpids_fs" {
  exec_iprof_vpids fs
  [ "$(calc_iprof_vpids fs)" -eq 2 ]
}

@test "sync_daemon_fs_launching_mpi_app" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  RUN_FS_DAEMON -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}

@test "sync_daemon_mpi" {
  RUN_MPI_DAEMON -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_mpi" {
  RUN_MPI_DAEMON -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

@test "sync_daemon_mpi_launching_mpi_app" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  RUN_MPI_DAEMON -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}
