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

@test "sync_daemon_fs" {
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_fs" {
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

calc_iprof_vpids() {
  rm -rf $1_traces

  THAPI_SYNC_DAEMON=$1 THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 $IPROF --no-analysis --trace-output $1_traces -- ./mpi_helloworld > /dev/null

  dir=$(ls -d -1 ./$1_traces/**)
  vpids=$($BBT -c $dir | sed -e "s/ - /, /g" | sed -e "s/,/\n/g" | grep vpid | sort | uniq | wc -l)
  echo $vpids
}

@test "iprof_vpids_mpi" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld

  vpids=$(calc_iprof_vpids mpi)
  [ "$vpids" -eq 2 ]
}

@test "iprof_vpids_fs" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld

  vpids=$(calc_iprof_vpids fs)
  [ "$vpids" -eq 2 ]
}

@test "sync_daemon_fs_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}

@test "sync_daemon_mpi" {
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_mpi" {
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

@test "sync_daemon_mpi_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=$(get_unique_jobid) timeout 40s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}
