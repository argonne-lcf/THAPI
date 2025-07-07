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

launch_mpi() {
  # - THAPI_JOBID required as no launcher are available on the CI who define MPIJOBID,
  #   each JOBID need to be uniqu per invocation
  # - timeout just to avoid burning too much hours when bug are introduced 
  THAPI_JOBID=${BATS_TEST_NAME}.${RANDOM} timeout 40s $MPIRUN "$@"
}

# THAPI_SYNC_DAEMON=fs Tests

@test "sync_daemon_fs" {
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_fs" {
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 $IPROF --no-analysis --trace-output "${BATS_TEST_NAME}" -- $THAPI_TEST_BIN
   # Count VPID
   [ $($BBT -c "${BATS_TEST_NAME}" | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_fs_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}

# THAPI_SYNC_DAEMON=MPI Tests

@test "sync_daemon_mpi" {
   THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_mpi" {
   THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 $IPROF --no-analysis --trace-output "${BATS_TEST_NAME}" -- $THAPI_TEST_BIN
   # Count VPID
   [ $($BBT -c "${BATS_TEST_NAME}" | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_mpi_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}
